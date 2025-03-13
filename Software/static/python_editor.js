var doc = document.getElementById('pythonDiv').contentWindow.document;
var editor = CodeMirror.fromTextArea(document.getElementById("python-in"), {
    theme: "material-darker",
    styleActiveLine: true,
    lineNumbers: true,
    matchBrackets: true,
    autoCloseBrackets: true,
    autoCloseTags: true,
    mode: "python",
});


var containerArea = document.getElementById('container');
var pythonDiv = document.getElementById('pythonDiv');
function onresize(e){
    // Compute the absolute coordinates and dimensions of containerArea.
    var element = containerArea;
    var x = 0;
    var y = 0;
    do {
      x += element.offsetLeft;
      y += element.offsetTop;
      element = element.offsetParent;
    } while (element);
    // Python
    pythonDiv.style.left = x + 'px';
    pythonDiv.style.top = y + 'px';
    pythonDiv.style.width = containerArea.offsetWidth + 'px';
    pythonDiv.style.height = containerArea.offsetHeight + 'px';

    console.log('resize');
};
window.addEventListener('resize', onresize, false);
onresize();
