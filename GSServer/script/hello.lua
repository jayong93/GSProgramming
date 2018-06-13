myId = 9999999999;

function set_my_id( id )
    myId = id;
end

function player_moved( player_id, x, y )
    my_x, my_y = c_get_my_pos(myId);
    if my_x == x and my_y == y then
        c_send_message(player_id, myId, "Hello");
    end
end
