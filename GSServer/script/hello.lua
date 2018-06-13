myId = 9999999999;
move_count = 0;

function set_my_id( id )
    myId = id;
end

function player_moved( player_id, x, y )
    local my_x, my_y = c_get_my_pos(myId);
    if my_x == x and my_y == y then
        c_send_message(myId, "Hello");
        move_count = 1;
        c_send_random_move(myId, 0);
    end
end

function random_move()
    local dx = c_get_random_num(-1, 1);
    local dy = 0;
    if dx == 0 then
        while dy == 0 do
            dy = c_get_random_num(-1, 1);
        end
    end
    c_move(myId, dx, dy);
    if move_count < 3 then
        c_send_random_move(myId, 1000);
        move_count = move_count + 1;
    else
        c_send_message(myId, "Bye!");
    end
end
