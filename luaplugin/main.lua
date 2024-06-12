local pluginName     = select(1,...);
local componentName  = select(2,...); 
local signalTable    = select(3,...);
local my_handle      = select(4,...);

-- ==========================================
-- Struct packing/unpacking functions
-- ==========================================
local log2 = math.log(2)
local function frexp(x)
	if x == 0 then return 0.0,0.0 end
	local e = math.floor(math.log(math.abs(x)) / log2)
	if e > 0 then
		-- Why not x / 2^e? Because for large-but-still-legal values of e this
		-- ends up rounding to inf and the wheels come off.
		x = x * 2^-e
	else
		x = x / 2^e
	end
	-- Normalize to the range [0.5,1)
	if math.abs(x) >= 1.0 then
		x,e = x/2,e+1
	end
	return x,e
end
local function ldexp(x, exp)
	return x * 2^exp
end
local function pack(format, ...)
  local stream = {}
  local vars = {...}
  local endianness = true

  for i = 1, format:len() do
	local opt = format:sub(i, i)

	if opt == '<' then
	  endianness = true
	elseif opt == '>' then
	  endianness = false
	elseif opt:find('[bBhHiIlL]') then
	  local n = opt:find('[hH]') and 2 or opt:find('[iI]') and 4 or opt:find('[lL]') and 8 or 1
	  local val = tonumber(table.remove(vars, 1))

	  local bytes = {}
	  for j = 1, n do
		table.insert(bytes, string.char(val % (2 ^ 8)))
		val = math.floor(val / (2 ^ 8))
	  end

	  if not endianness then
		table.insert(stream, string.reverse(table.concat(bytes)))
	  else
		table.insert(stream, table.concat(bytes))
	  end
	elseif opt:find('[fd]') then
	  local val = tonumber(table.remove(vars, 1))
	  local sign = 0

	  if val < 0 then
		sign = 1
		val = -val
	  end

	  local mantissa, exponent = frexp(val)
	  if val == 0 then
		mantissa = 0
		exponent = 0
	  else
		mantissa = (mantissa * 2 - 1) * ldexp(0.5, (opt == 'd') and 53 or 24)
		exponent = exponent + ((opt == 'd') and 1022 or 126)
	  end

	  local bytes = {}
	  if opt == 'd' then
		val = mantissa
		for i = 1, 6 do
		  table.insert(bytes, string.char(math.floor(val) % (2 ^ 8)))
		  val = math.floor(val / (2 ^ 8))
		end
	  else
		table.insert(bytes, string.char(math.floor(mantissa) % (2 ^ 8)))
		val = math.floor(mantissa / (2 ^ 8))
		table.insert(bytes, string.char(math.floor(val) % (2 ^ 8)))
		val = math.floor(val / (2 ^ 8))
	  end

	  table.insert(bytes, string.char(math.floor(exponent * ((opt == 'd') and 16 or 128) + val) % (2 ^ 8)))
	  val = math.floor((exponent * ((opt == 'd') and 16 or 128) + val) / (2 ^ 8))
	  table.insert(bytes, string.char(math.floor(sign * 128 + val) % (2 ^ 8)))
	  val = math.floor((sign * 128 + val) / (2 ^ 8))

	  if not endianness then
		table.insert(stream, string.reverse(table.concat(bytes)))
	  else
		table.insert(stream, table.concat(bytes))
	  end
	elseif opt == 's' then
	  table.insert(stream, tostring(table.remove(vars, 1)))
	  table.insert(stream, string.char(0))
	elseif opt == 'c' then
	  local n = format:sub(i + 1):match('%d+')
	  local str = tostring(table.remove(vars, 1))
	  local len = tonumber(n)
	  if len <= 0 then
		len = str:len()
	  end
	  if len - str:len() > 0 then
		str = str .. string.rep(' ', len - str:len())
	  end
	  table.insert(stream, str:sub(1, len))
	  i = i + n:len()
	end
  end

  return table.concat(stream)
end
local function unpack(format, stream, pos)
  local vars = {}
  local iterator = pos or 1
  local endianness = true

  for i = 1, format:len() do
	local opt = format:sub(i, i)

	if opt == '<' then
	  endianness = true
	elseif opt == '>' then
	  endianness = false
	elseif opt:find('[bBhHiIlL]') then
	  local n = opt:find('[hH]') and 2 or opt:find('[iI]') and 4 or opt:find('[lL]') and 8 or 1
	  local signed = opt:lower() == opt

	  local val = 0
	  for j = 1, n do
		local byte = string.byte(stream:sub(iterator, iterator))
		if endianness then
		  val = val + byte * (2 ^ ((j - 1) * 8))
		else
		  val = val + byte * (2 ^ ((n - j) * 8))
		end
		iterator = iterator + 1
	  end

	  if signed and val >= 2 ^ (n * 8 - 1) then
		val = val - 2 ^ (n * 8)
	  end

	  table.insert(vars, math.floor(val))
	elseif opt:find('[fd]') then
	  local n = (opt == 'd') and 8 or 4
	  local x = stream:sub(iterator, iterator + n - 1)
	  iterator = iterator + n

	  if not endianness then
		x = string.reverse(x)
	  end

	  local sign = 1
	  local mantissa = string.byte(x, (opt == 'd') and 7 or 3) % ((opt == 'd') and 16 or 128)
	  for i = n - 2, 1, -1 do
		mantissa = mantissa * (2 ^ 8) + string.byte(x, i)
	  end

	  if string.byte(x, n) > 127 then
		sign = -1
	  end

	  local exponent = (string.byte(x, n) % 128) * ((opt == 'd') and 16 or 2) + math.floor(string.byte(x, n - 1) / ((opt == 'd') and 16 or 128))
	  if exponent == 0 then
		table.insert(vars, 0.0)
	  else
		mantissa = (ldexp(mantissa, (opt == 'd') and -52 or -23) + 1) * sign
		table.insert(vars, ldexp(mantissa, exponent - ((opt == 'd') and 1023 or 127)))
	  end
	elseif opt == 's' then
	  local bytes = {}
	  for j = iterator, stream:len() do
		if stream:sub(j,j) == string.char(0) or  stream:sub(j) == '' then
		  break
		end

		table.insert(bytes, stream:sub(j, j))
	  end

	  local str = table.concat(bytes)
	  iterator = iterator + str:len() + 1
	  table.insert(vars, str)
	elseif opt == 'c' then
	  local n = format:sub(i + 1):match('%d+')
	  local len = tonumber(n)
	  if len <= 0 then
		len = table.remove(vars)
	  end

	  table.insert(vars, stream:sub(iterator, iterator + len - 1))
	  iterator = iterator + len
	  i = i + n:len()
	end
  end

  table.insert(vars, iterator)

  return table.unpack(vars)
end

-- ==========================================
-- Wrapper for struct packing/unpacking
-- ==========================================
local Stream = {}
Stream.__index = Stream

function Stream:new(stream)
	local o = {}
	setmetatable(o, Stream)
	o.stream = stream
	o.pos = 1
	return o
end

function Stream:read(format)
	local results = {unpack(format, self.stream, self.pos)}
	local n = #results

	-- Update position data
	self.pos = results[n]
	return table.unpack(results, 1, n - 1)
end

----------------------------------------------
------------------ Enums ---------------------
----------------------------------------------
local REQ_ENCODERS = 0x8000
local RESP_ENCODERS = 0x8001
-- local UNUSED = 0x8002
local UPDATE_MA_ENCODER = 0x8003
local UPDATE_MA_MASTER = 0x8004
local PRESS_MA_PLAYBACK_KEY = 0x8005
local PRESS_MA_SYSTEM_KEY = 0x8006
local PACKET_TYPE_END = 0x8007

local KeyType_CLEAR = 0x10101010
local KeyType_STORE = KeyType_CLEAR + 1
local KeyType_UPDATE = KeyType_STORE + 1
local KeyType_ASSIGN = KeyType_UPDATE + 1
local KeyType_MOVE = KeyType_ASSIGN + 1
local KeyType_OOPS = KeyType_MOVE + 1
local KeyType_EDIT = KeyType_OOPS +1
local KeyType_DELETE = KeyType_EDIT +1
local KeyType_ESC = KeyType_DELETE +1

local EncoderType_x100 = 0x100
local EncoderType_x200 = 0x200
local EncoderType_x300 = 0x300
local EncoderType_x400 = 0x400
local EncoderType_None = 0x500

----------------------------------------------
----------------- Helpers --------------------
----------------------------------------------
local function GetEncoder(page_num, channel_num, encoderType)
	local page = Root().ShowData.DataPools.Default.Pages:Ptr(page_num)
	if not page then
		return
	end
	local ch = page:Ptr(channel_num + encoderType)
	if not ch then
		return
	end
	return ch
end

----------------------------------------------
------------------ Funcs ---------------------
----------------------------------------------
local function ExtractEncoderRequest(conn)
	-- IPC_STRUCT {
	-- 	unsigned int page;
	-- 	unsigned int channel;
	-- } EncoderRequest[8];

	local encoders = {}
	local pages = {} -- Trak which pages have been requested (unique pages)

	for i = 1, 8 do
		local page, channel = conn.stream:read("<II")
		-- Printf("Page: " .. tostring(page)   .. " Channel: " .. tostring(channel))
		local req = {}
		req["page"] = page
		req["channel"] = channel
		table.insert(encoders, req)

		if not pages[page] then
			pages[page] = true
		end
	end

	return encoders, pages
end

local function GetExecutersFromChannel(page, channel)
	local _400, _300, _200, _100
	local executerTable = {}
	local bchActive = 0
	for i = 0, 3 do
		local channel_id = channel + (i * 100)
		local executor = page:Ptr(channel_id)
		if executor then
			bchActive = 1
			if i == 3 then
				_400 = {}
				_400["exec"] = executor
				_400["type"] = EncoderType_x400
			elseif i == 2 then
				_300 = {}
				_300["exec"] = executor
				_300["type"] = EncoderType_x300
			elseif i == 1 then
				_200 = {}
				_200["exec"] = executor
				_200["type"] = EncoderType_x200
			elseif i == 0 then
				_100 = {}
				_100["exec"] = executor
				_100["type"] = EncoderType_x100
			end
		end
	end

	table.insert(executerTable, _400)
	table.insert(executerTable, _300)
	table.insert(executerTable, _200)
	table.insert(executerTable, _100)

	return bchActive, executerTable
end

local function PrepareEncoderData(connection, encoderObj)
	Printf("Preparing encoder data")
	-- struct ChannelData  {
	-- 	uint16_t page;
	-- 	uint8_t channel; // eg x01, x02, x03
	-- 	struct {
	-- 		bool isActive;
	-- 		char key_name[8];
	-- 		float value;
	-- 	} Encoders[3]; // 4xx, 3xx, 2xx encoders
	-- 	bool keysActive[4]; // 4xx, 3xx, 2xx, 1xx keys are being used
	-- };
	-- Printf("encoderObj -- page: " .. tostring(encoderObj.page) .. " channel: " .. tostring(encoderObj.channel))
	local packet_data = pack("<HB", encoderObj.page, encoderObj.channel) 

	-- Encoders
	for i=1, 3 do
		Printf("Encoder index: " .. tostring(i))
		local encoder = encoderObj.unsafeEncoders[i]
		local exec = encoder["exec"]
		if encoder == nil or exec["FADER"] == "" then
			-- Printf("Encoder is nil on page " .. tostring(encoderObj.page) .. " channel " .. tostring(encoderObj.channel))
			packet_data = packet_data .. pack("<HBc8f", EncoderType_None, 0, "        ", 0)
			Printf("Encoder is nil on page " .. tostring(encoderObj.page) .. " channel " .. tostring(encoderObj.channel))
		else
			if exec["FADER"] == "" then
				-- Printf("empty text")
			end
			-- Printf("Encoder is not nil on page " .. tostring(encoderObj.page) .. " channel " .. tostring(encoderObj.channel))
			-- Printf("Encoder name: " .. encoder["FADER"] .. " value: " .. encoder:GetFader({}))
			Printf("Encoder name: " .. exec["FADER"] .. " value: " .. exec:GetFader({}))
			packet_data = packet_data .. pack("<HBc8f", encoder["type"] , 1, string.sub(exec["FADER"], 1, 8), exec:GetFader({}))
		end
	end

	-- keysActive
	for i=1, 4 do
		local encoder = encoderObj.unsafeEncoders[i]
		if encoder == nil or encoder["exec"]["KEY"] == "" then
			packet_data = packet_data .. pack("<B", 0)
		else
			packet_data = packet_data .. pack("<B", 1)
		end
	end

	return packet_data
end

local function HandleSendingEncoderData(connection, seq)
	-- Collect all requests, and create a set of pages to request.
	-- The later is to avoid loading the same page multiple times if 
	-- the page is not valid.
	local requests, unique_pages = ExtractEncoderRequest(connection)


	-- Get all pages requested, and cache their pointers
	local page_cache = {}
	for k, v in pairs(unique_pages) do
		-- Using 'default' datapool for now. Is this an issue in the future?
		local page = Root().ShowData.DataPools.Default.Pages:Ptr(k)
		page_cache[k] = page
	end

	-- Collect all encoder data
	local arrbEncoderActive = {}
	local arrEncoders = {}
	for k, v in ipairs(requests) do
		-- Printf("Processing request -- page " .. tostring(v["page"]) .. " channel " .. tostring(v["channel"]))
		local page_ptr = page_cache[v["page"]]
		if page_ptr == nil then
			table.insert(arrbEncoderActive, 0)
			-- Printf("[" .. tostring(k) .. "] false")
			goto continue
		end

		local bchActive, encoders = GetExecutersFromChannel(page_ptr, v["channel"])
		table.insert(arrbEncoderActive, bchActive)
		if bchActive == 1 then
			local wrappedEncoder = {
				page = v["page"],
				channel = v["channel"],
				unsafeEncoders = encoders -- May contain nil values
			}
			table.insert(arrEncoders, wrappedEncoder)
		end
		
		::continue::
  	end

	-- ===========================================================
	-- =================== IPC::IPCHeader ========================
	-- ===========================================================
	local packet_data = pack("<II", RESP_ENCODERS, seq) 

	-- ===========================================================
	-- ========= IPC::PlaybackRefresh::ChannelMetadata ===========
	-- ===========================================================
	packet_data = packet_data .. pack("<f", Root().ShowData.Masters.Grand.Master:GetFader({}))
	for k, v in ipairs(arrbEncoderActive) do
		packet_data = packet_data .. pack("<B", v)
	end

	-- ===========================================================
	-- =============== IPC::PlaybackRefresh::Data ================
	-- ===========================================================
	for k, v in ipairs(arrEncoders) do
		packet_data = packet_data .. PrepareEncoderData(connection, v)
	end
	SendPacket(connection, packet_data)
end

local function HandleUpdatingLocalMasterEncoder(connection, seq)
	local value = connection.stream:read("<f")
	Root().ShowData.Masters.Grand.Master:SetFader({value=value})
end

local function HandleUpdatingLocalEncoder(connection, seq)
	local _page, channel, encoderType, value = connection.stream:read("<HBHf")
	encoderType = encoderType - 100 -- Convert to 0-based index, kinda confusing
	local ch = GetEncoder(_page, channel, encoderType)
	if not ch then
		return
	end
	ch:SetFader({value=value})
end

local function HandlePressingPlaybackKey(connection, seq)
	local page, channel, encoder_type, down = connection.stream:read("<HBHB")
	local ch = GetEncoder(page, channel, encoder_type)
	if not ch then
		return
	end
	if down == 1 then
		Cmd(tostring(ch["KEY"] .. " Press " .. tostring(ch["EXEC"])))
	else
		Cmd(tostring(ch["KEY"] .. " Unpress " .. tostring(ch["EXEC"])))
	end

end

local function HandlePressingSystemKey(connection, seq)
	local keyType, down = connection.stream:read("<IB")
	if down == 1 then
		return
	end

	if keyType == KeyType_CLEAR then
		Cmd("Clear")
		return
	elseif keyType == KeyType_ASSIGN then
		Root().GraphicsRoot.PultCollect[1].DisplayCollect[1].CmdLineSection.DisplayCommandLine.content = "Assign"
		return
	elseif keyType == KeyType_STORE then
		Root().GraphicsRoot.PultCollect[1].DisplayCollect[1].CmdLineSection.DisplayCommandLine.content = "Store"
		return
	elseif keyType == KeyType_UPDATE then
		Root().GraphicsRoot.PultCollect[1].DisplayCollect[1].CmdLineSection.DisplayCommandLine.content = "Update"
		return
	elseif keyType == KeyType_MOVE then
		Root().GraphicsRoot.PultCollect[1].DisplayCollect[1].CmdLineSection.DisplayCommandLine.content = "Move"
		return
	elseif keyType == KeyType_OOPS then
		Cmd("Oops")
		return
	elseif keyType == KeyType_EDIT then
		Root().GraphicsRoot.PultCollect[1].DisplayCollect[1].CmdLineSection.DisplayCommandLine.content = "Edit"
		return
	elseif keyType == KeyType_DELETE then
		Root().GraphicsRoot.PultCollect[1].DisplayCollect[1].CmdLineSection.DisplayCommandLine.content = "Delete"
		return
	elseif keyType == KeyType_ESC then
		Root().GraphicsRoot.PultCollect[1].DisplayCollect[1].CmdLineSection.DisplayCommandLine.content = ""
		return
	end

end

local function HandleConnection(socket, ip, port, data)
	if not data then return end

	local stream = Stream:new(data)
	local connection = {
		conn = socket,
		ip = ip,
		port = port,
		stream = stream
	}
	pkt_type, seq = stream:read("<II")
	-- Handlers
	assert(
		pkt_type >= REQ_ENCODERS and pkt_type < PACKET_TYPE_END,
		"Invalid packet type"
	)
	if pkt_type == REQ_ENCODERS then
		HandleSendingEncoderData(connection, seq)
	elseif pkt_type == UPDATE_MA_ENCODER then
		HandleUpdatingLocalEncoder(connection, seq)
	elseif pkt_type == UPDATE_MA_MASTER then
		HandleUpdatingLocalMasterEncoder(connection, seq)
	elseif pkt_type == PRESS_MA_PLAYBACK_KEY then
		HandlePressingPlaybackKey(connection, seq)
	elseif pkt_type == PRESS_MA_SYSTEM_KEY then
		HandlePressingSystemKey(connection, seq)
	end
end

local function printmsg()
	Printf("Change event")
end

local function BeginListening()
	HookObjectChange(printmsg, Root().ShowData.DataPools.Default.Pages, my_handle:Parent())

	local socket = require("socket")
	local udp = assert(socket.udp4())
	local port = 9000

	udp:settimeout(0)
	assert(udp:setsockname("*", port))

	Printf("Entering loop")
	while true do
		local data, ip, port = udp:receivefrom()
		if data then
			HandleConnection(udp, ip, port, data)
		else
			coroutine.yield(0)
		end
	end
end

function SendPacket(connection_object, packet_data)
	-- Printf("Sending data")
	connection_object.conn:sendto(packet_data, connection_object.ip, connection_object.port)
end

return BeginListening