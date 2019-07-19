local exports = {}
exports.name = "workerui"
exports.version = "0.0.1"
exports.description = "worker-ui plugin"
exports.license = "The BSD 3-Clause License"
exports.author = { name = "Bletch" }

local console = exports

function quoted_string_split(text)
	local result = {}
	local e = 0
	local i = 1
	while true do
		local b = e+1
		b = text:find("%S",b)
		if b==nil then break end
		if text:sub(b,b)=="'" then
			e = text:find("'",b+1)
			b = b+1
		elseif text:sub(b,b)=='"' then
			e = text:find('"',b+1)
			b = b+1
		else
			e = text:find("%s",b+1)
		end
		if e==nil then e=#text+1 end

		result[i] = text:sub(b,e-1)
		i = i + 1
	end
	return result
end

function dump(o)
   if type(o) == 'table' then
      local s = '{ '
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = '"'..k..'"' end
         s = s .. '['..k..'] = ' .. dump(v) .. ','
      end
      return s .. '} '
   else
      return tostring(o)
   end
end

function toboolean(str)
	str = str:lower()
	return str == "1" or str == "on" or str == "true"
end

function speed_text()
	-- show speed percentage first
	local text = tostring(math.floor(manager:machine():video():speed_percent() * 100 + 0.5)) .. "%"

	-- then show effective frameskip, if more than zero
	local effective_frameskip = manager:machine():video():effective_frameskip()
	if effective_frameskip > 0 then
		text = text .. " (frameskip " .. tostring(effective_frameskip) .. "/10)"
	end
	return text
end

function emit_status()
	print("<status");
	print("\tphase=\"running\"");
	print("\tpolling_input_seq=\"false\"");
	print("\tnatural_keyboard_in_use=\"false\"");
	if manager:machine() then
		print("\tpaused=\"" .. tostring(manager:machine().paused) .. "\"");
	else
		print("\tstartup_text=\"Initializing...\"");
	end
	print(">");

	print("\t<video");
	print("\t\tspeed_text=\"" .. speed_text() .. "\"");
	if manager:machine() then
		print("\t\tframeskip=\"" .. tostring(manager:machine():video().frameskip) .. "\"");
		print("\t\tthrottled=\"" .. tostring(manager:machine():video().throttled) .. "\"");
		print("\t\tthrottle_rate=\"" .. tostring(manager:machine():video().throttle_rate) .. "\"");
	end
	print("\t/>");

	if manager:machine() then
		print("\t<sound");
		print("\t\tattenuation=\"" .. tostring(manager:machine():sound().attenuation) .. "\"");
		print("\t/>");
	end

	print("</status>");
end

-- EXIT command
function command_exit(args)
	manager:machine():exit()
	print "OK ### Exit scheduled"
end

-- PING command
function command_ping(args)
	print "OK STATUS ### Ping... pong..."
	emit_status()
end

-- SOFT_RESET command
function command_soft_reset(args)
	manager:machine():soft_reset()
	print("OK ### Soft Reset Scheduled")
end

-- HARD_RESET command
function command_hard_reset(args)
	manager:machine():hard_reset()
	print("OK ### Hard Reset Scheduled")
end

-- PAUSE command
function command_pause(args)
	emu.pause()
	print("OK STATUS ### Paused")
	emit_status()
end

-- RESUME command
function command_resume(args)
	emu.unpause()
	print("OK STATUS ### Resumed")
	emit_status()
end

-- THROTTLED command
function command_throttled(args)
	manager:machine():video().throttled = toboolean(args[2])
	print("OK STATUS ### Throttled set to " .. tostring(manager:machine():video().throttled))
	emit_status()
end

-- THROTTLE_RATE command
function command_throttle_rate(args)
	manager:machine():video().throttle_rate = tonumber(args[2])
	print("OK STATUS ### Throttle rate set to " .. tostring(manager:machine():video().throttle_rate))
	emit_status()
end

-- FRAMESKIP command
function command_frameskip(args)
	local frameskip
	if (args[2]:lower() == "auto") then
		frameskip = -1
	else
		frameskip = tonumber(args[2])
	end
	manager:machine():video().frameskip = frameskip
	print("OK STATUS ### Frame skip rate set to " .. tostring(manager:machine():video().frameskip))
	emit_status()
end

-- INPUT command
function command_input(args)
	emu.keypost(args[2])
	print("OK ### Text inputted")
end

-- PASTE command
function command_paste(args)
	emu.paste()
	print("OK ### Text inputted from clipboard")
end

-- SET_ATTENUATION command
function command_set_attenuation(args)
	manager:machine():sound().attenuation = tonumber(args[2])
	print("OK STATUS ### Sound attenuation set to " .. tostring(manager:machine():sound().attenuation))
	emit_status()
end

-- not implemented command
function command_nyi(args)
	print("ERROR ### Command '" .. args[1] .. "' not yet implemeted")
end

-- unknown command
function command_unknown(args)
	print("ERROR ### Unrecognized command '" .. args[1] .. "'")
end

-- command list
local commands =
{
	["exit"]						= command_exit,
	["ping"]						= command_ping,
	["soft_reset"]					= command_soft_reset,
	["hard_reset"]					= command_hard_reset,
	["throttled"]					= command_throttled,
	["throttle_rate"]				= command_throttle_rate,
	["frameskip"]					= command_frameskip,
	["pause"]						= command_pause,
	["resume"]						= command_resume,
	["input"]						= command_input,
	["paste"]						= command_paste,
	["set_attenuation"]				= command_set_attenuation,
	["set_natural_keyboard_in_use"]	= command_nyi,
	["state_load"]					= command_nyi,
	["state_save"]					= command_nyi,
	["save_snapshot"]				= command_nyi,
	["load"]						= command_nyi,
	["unload"]						= command_nyi,
	["create"]						= command_nyi,
	["seq_poll_start"]				= command_nyi,
	["seq_set_default"]				= command_nyi,
	["seq_clear"]					= command_nyi
}

-- invokes a command line
function invoke_command_line(line)
	-- invoke the appropriate command
	local invocation = (function()
		local args = quoted_string_split(line)
		if (commands[args[1]:lower()]) then
			commands[args[1]:lower()](args)
		else
			command_unknown(args)
		end
	end)

	local status, err = pcall(invocation)
	if (not status) then
		print("ERROR ## Lua runtime error " .. tostring(err.code))
	end
end

function console.startplugin()
	-- start a thread to read from stdin
	local scr = "return io.read()"
	local conth = emu.thread()
	conth:start(scr);

	-- we want to hold off until the prestart event; register a handler for it
	local prestarted = false
	emu.register_prestart(function()
		if not prestarted then
			-- prestart has been invoked; we're ready for commands
			emu.pause()
			print("OK STATUS ### Emulation commenced; ready for commands")
			emit_status()
			prestarted = true
		end
	end)

	-- register another handler to handle commands after prestart
	emu.register_periodic(function()
		if (prestarted and not (conth.yield or conth.busy)) then
			-- invoke the command line
			invoke_command_line(conth.result)

			-- continue on reading
			conth:start(scr)
		end
	end)
end

return exports
