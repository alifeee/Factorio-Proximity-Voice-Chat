script.on_nth_tick(5, function()
    for index, player in pairs(game.connected_players) do
        local info = "XYZ, Player, sUrface, Server\n" .. "x: " .. player.position.x .. "\n" .. "y: " ..
                         player.position.y .. "\n" .. "z: " .. 0 .. "\n" .. "p: " .. index .. "\n" .. "u: " ..
                         player.surface.index .. "\n" .. "s: " .. game.get_player(1).name .. "\n"
        game.write_file("mumble_positional-audio_information.txt", info, false, player.index)
    end
end)
