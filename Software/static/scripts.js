var motors = [0, 0]
var int
var line = false
var colors = false

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
