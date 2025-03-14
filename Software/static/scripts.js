var motors = [0, 0]
var int
var line = false
var colors = false
var blockly_edit = true

var connection = new WebSocket('ws:'+location.hostname+':8010/ws');
var keysdown = {};
var wasd_ctrl = false

int = setInterval(() => { connection.send("get_data"); console.log("get_data") }, 1000);

// keydown handler
$(document).keydown(function(e){
    // Do we already know it's down?
    if (keysdown[e.keyCode] || !wasd_ctrl) {
        // Ignore it
        return;
    }

    // Remember it's down
    keysdown[e.keyCode] = true;

    // Do our thing
    sendkeys(connection, keysdown);
});

// keyup handler
$(document).keyup(function(e){
    if (!wasd_ctrl) {
        // Ignore it
        return;
    }
    // Remove this key from the map
    delete keysdown[e.keyCode];

    // Do our thing
    sendkeys(connection, keysdown);
});

connection.onmessage = function(e){
    console.log(e.data);
    var data_json = JSON.parse(e.data);
    document.getElementById(data_json.id).textContent = data_json.value;
}

$(document).on("click", function (e) {
    var target = $(e.target);
    if(target.attr('id') === "camera_feed"){
        wasd_ctrl = true
    }else if(wasd_ctrl){
        wasd_ctrl = false
        keysdown = {};
        // Do our thing
        sendkeys(connection, keysdown);
    }
});

function sendkeys(connection, keys){
    motors = [0, 0]

    motors[0] += ("87" in keys) * 127 + ("68" in keys) * 63;  // w, d
    motors[0] -= ("83" in keys) * 127 + ("65" in keys) * 63;  // s, a
    motors[1] += ("87" in keys) * 127 + ("65" in keys) * 63;  // w, a
    motors[1] -= ("83" in keys) * 127 + ("68" in keys) * 63;  // s, d

    // Do our thing
    connection.send("motors:" + motors);
}

function sendslid(connection, id1, id2){
    var left = document.getElementById(id1);
    var right = document.getElementById(id2);

    motors = [left.value, right.value];

    document.getElementById("left_m").innerHTML = left.value;
    document.getElementById("right_m").innerHTML = right.value;

    // Do our thing
    connection.send("motors:" + motors);
}

function linechange(connection, linebox){
    if (linebox.checked) {
        connection.send("line:1");
        line = true;
    } else {
        connection.send("line:0");
        line = false;
    }
}

function colorschange(connection, colorsbox){
    if (colorsbox.checked) {
        connection.send("colors:1");
        colors = true;
    } else {
        connection.send("colors:0");
        colors = false;
    }
}

function change_editor(button){
    if (blockly_edit) {
        blockly_edit = false;
        $('.CodeMirror').show();
        $('.blocklyContents').hide();
        button.textContent = "Blockly editor";
    } else {
        blockly_edit = true;
        $('.CodeMirror').hide();
        $('.blocklyContents').show();
        button.textContent = "Python editor";
    }
}
