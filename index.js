var SilkCodec = require('bindings')('silk_codec_addon').SilkCodec


// input/output audio data is little endian
class SilkEncoder{
  constructor(...props){
    this.codec = new SilkCodec("encoder", ...props)
  }

  // encode pcm into silk (silk package size 20ms)
  encode = function(...props){
    return this.codec.encode(...props)
  }
}

// input/output audio data is little endian
class SilkDecoder{
  constructor(...props){
    this.codec = new SilkCodec("decoder", ...props)
  }

  // decode silk into pcm
  decode = function(...props){
    return this.codec.decode(...props)
  }
}

module.exports = {SilkEncoder, SilkDecoder}
