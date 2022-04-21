var SilkEncoder = require('../').SilkEncoder
var SilkDecoder = require('../').SilkDecoder
var fs = require("fs");

function testEncoder(pcmFile, silkFile)
{
    var encoder = new SilkEncoder(8000, 16, 1);
    var data = fs.readFileSync(pcmFile);
    console.info("pcm file size:", data.length)

    // encode
    silk_data = encoder.encode(data.buffer);

    // write to silk file

    // write silk file header
    var header = Buffer.from("#!SILK_V3")
    fs.writeFileSync(silkFile, header);
    
    // write silk data
    fs.appendFileSync(silkFile, silk_data);
}

function testDecoder(silkFile, pcmFile)
{
    var decoder = new SilkDecoder(8000, 16, 1);
    var data = fs.readFileSync(silkFile);
    console.info("silk file size:", data.length)

    // check header
    var header = Buffer.from("#!SILK_V3")
    if ( data.compare(header, 0, header.length, 0, header.length ) != 0){
        console.error("invalid silk file")
        return
    }

    // decode
    sub = Buffer.from(data.subarray(header.length))
    pcm_data = decoder.decode(sub.buffer);

    // write to pcm file
    fs.writeFileSync(pcmFile, pcm_data);
}

testEncoder("8000_16bit_1channel.pcm", "silk_out.silk")
//testDecoder("silk_out.silk", "pcm_out.pcm")
testDecoder("8000_16bit_1channel_20ms.silk", "pcm_out.pcm");