mkdir static/lib
cd static/lib

wget https://github.com/google/blockly/releases/download/blockly-v11.2.1/blockly-11.2.1.tgz
wget https://code.jquery.com/jquery-1.10.2.min.js
wget https://codemirror.net/5/codemirror.zip

tar -xzf blockly-11.2.1.tgz
mv package blockly
rm blockly-11.2.1.tgz

unzip -q codemirror.zip
mv codemirror-5.65.18 codemirror
rm codemirror.zip

cd ../..