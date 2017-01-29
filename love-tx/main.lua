local socket = require("socket")
local util = require("util")
local bit = require("bit")

---

local PPM_IP = "192.168.8.101"
local PPM_PORT = 5620

---

local client
local joystick
local text = "OK"
local failsafe = true

---

local function centreprint(msg, y)
	local font = love.graphics.getFont()

	local w = font:getWidth(msg)
	local h = font:getHeight()

	local x = (love.graphics.getWidth()/2) - (w/2)
	local y = y or (love.graphics.getHeight()/2) - (h/2)

	love.graphics.print(msg, x, y)
end

---

-- Convert number to big-endian u16 (as string).
local function n_to_u16(n)
	local high = bit.band(bit.rshift(n, 8), 0xFF)
	local low = bit.band(n, 0xFF)
	
	return string.char(high, low)
end

-- Convert from the range [-1.0, 1.0]
-- to [0.0, 1.0]
local function zeromid_to_zerolow(n)
	return (n + 1) / 2
end

-- Convert list of channels in [-1.0, 1.0] to
-- the bytes to be sent in a packet.
local function channels_to_bytes(channels)
	local str = {}
	
	for i, value in ipairs(channels) do
		local norm = zeromid_to_zerolow(value)
		local high, low = n_to_u16(math.floor(norm * 65535))
		
		str[#str + 1] = high
		str[#str + 1] = low
	end
	
	return table.concat(str)
end

---

function love.load()
	love.graphics.setFont(love.graphics.newFont(32))
	
	---
	
	joystick = love.joystick.getJoysticks()[1]
	
	if joystick then
		print("Using: " .. joystick:getName())
	else
		print("No joystick found!")
		love.event.quit() 
	end
	
	---
	
	client = socket.udp()
	client:settimeout(1)
	
	client:setpeername(PPM_IP, PPM_PORT)
end

function love.update(dt)
	local channels = {}
	local input_roll, input_pitch, input_yaw, input_throttle = joystick:getAxes()
	
	---
	
	text = ("Throttle: %.2f\nRoll: %.2f\nPitch: %.2f\nYaw: %.2f")
		:format(zeromid_to_zerolow(-input_throttle), input_roll, input_pitch, input_yaw)
	
	channels[1] = -input_throttle
	channels[2] = input_roll
	channels[3] = input_pitch
	channels[4] = input_yaw
	channels[5] = 0
	channels[6] = 0
	channels[7] = 0
	channels[8] = 0
	
	---
	
	if not failsafe then
		local n, err = client:send(channels_to_bytes(channels))

		if n then
			text = text .. "\nOK"
		else
			text = text .. "\nError: " .. err
		end
	else
		text = text .. "\nFAILSAFE"
	end
end

function love.focus(f)
	if not f then
		failsafe = true
	end
end

function love.keypressed(k)
	-- Force failsafe.
	if k == "space" then
		failsafe = true
		
	-- Reset failsafe.
	elseif k == "r" then
		failsafe = false
	end
end

function love.joystickpressed(j, b)
	if j == joystick and b == 1 then
		failsafe = true
	end
end

function love.draw()
	centreprint(text)
end

