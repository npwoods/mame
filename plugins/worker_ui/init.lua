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
	print("\t\tspeed_text=\"dummy\"");
	if manager:machine() then
		print("\t\tframeskip=\"" .. tostring(manager:machine():video().frameskip) .. "\"");
		print("\t\tthrottled=\"" .. tostring(manager:machine():video().throttled) .. "\"");
		print("\t\tthrottle_rate=\"" .. tostring(manager:machine():video().throttle_rate) .. "\"");
	end
	print("\t/>");

	if manager:machine() then
		print("\t<sound");
		print("\t\tattenuation=\"0\"");
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

-- not implemented command
function command_nyi(args)
	print("ERROR ### Command " .. args[1] .. " not yet implemeted")
end

-- unknown command
function command_unknown(args)
	print("ERROR ### Unrecognized command " .. args[1])
end

-- command list
local commands =
{
	["exit"]						= command_exit,
	["ping"]						= command_ping,
	["soft_reset"]					= command_soft_reset,
	["hard_reset"]					= command_hard_reset,
	["throttled"]					= command_nyi,
	["throttle_rate"]				= command_nyi,
	["frameskip"]					= command_nyi,
	["pause"]						= command_pause,
	["resume"]						= command_resume,
	["input"]						= command_nyi,
	["paste"]						= command_nyi,
	["set_attenuation"]				= command_nyi,
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

function console.startplugin()
	-- start a thread to read from stdin
	local scr = "return io.read()"
	local conth = emu.thread()
	conth:start(scr);

	-- we want to hold off until the prestart event; register a handler for it
	local prestarted = false
	emu.register_prestart(function()
		-- prestart has been invoked; we're ready for commands
		emu.pause()
		print("OK STATUS ### Emulation commenced; ready for commands")
		emit_status()
		prestarted = true
	end)

	-- register another handler to handle commands after prestart
	emu.register_periodic(function()
		if (prestarted and not (conth.yield or conth.busy)) then
			-- invoke the appropriate command
			local args = quoted_string_split(conth.result)
			if (commands[args[1]:lower()]) then
				commands[args[1]:lower()](args)
			else
				command_unknown(args)
			end

			-- continue on reading
			conth:start(scr)
		end
	end)
end

return exports
