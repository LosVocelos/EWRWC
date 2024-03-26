
var direction = [0, 0]
var int
var line = false
var colors = false

function sendkeys(connection, keys){
    direction = [0, 0]

    direction[1] += ("87" in keys) * 127;  // w
    direction[1] -= ("83" in keys) * 127;  // s
    direction[0] -= ("65" in keys) * 127;  // a
    direction[0] += ("68" in keys) * 127;  // d

    // Do our thing
    connection.send("motors:" + direction);
}

function sendslid(connection, id1, id2){
    var rot = document.getElementById(id1);
    var vel = document.getElementById(id2);

    direction = [rot.value, vel.value];

    document.getElementById("velocity").data = vel.value;
    document.getElementById("angle").data = rot.value;

    // Do our thing
    connection.send("motors:" + direction);
}

function linechange(connection, linebox){
    if (linebox.checked) {
        connection.send("line:1");
        line = true;
    } else {
        connection.send("line:0");
        line = false;
    }
    updateint(connection);
}

function colorschange(connection, colorsbox){
    if (colorsbox.checked) {
        connection.send("colors:1");
        colors = true;
    } else {
        connection.send("colors:0");
        colors = false;
    }
    updateint(connection);
}

function updateint(connection){
    if (line || colors) {
        int = setInterval(() => { connection.send("get_data"); console.log("get_data") }, 500);
    } else {
        clearInterval(int);
    }
}
