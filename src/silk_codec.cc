#include <stdio.h>
#include "silk_codec.h"
#include <chrono>
#include <thread>
#include <iostream>
#include "silk/interface/SKP_Silk_SDK_API.h"


#define MAX_INPUT_FRAMES        5
#define FRAME_LENGTH_MS         20
#define MAX_API_FS_KHZ          48

Napi::FunctionReference SilkCodec::constructor;

/* SwapEndian: convert a little endian int16 to a big endian int16 or vica verca
 * vec: int16 array
 * len: length of vec
 */
void SwapEndian(int16_t vec[],int len)
{
    for(int i = 0; i < len; i++){
        int16_t tmp = vec[i];
        uint8_t *p1 = (uint8_t *) &vec[i]; 
        uint8_t *p2 = (uint8_t *) &tmp;
        p1[0] = p2[1]; 
        p1[1] = p2[0];
    }
}

/* IsBigEndian: check system endian
 */
bool IsBigEndian()
{
    uint16_t n = 1;
    if( ((uint8_t*) &n)[0] == 1 ){
        return false;
    }
    return true;
}

void Short2LittleEndianBytes(short st, BYTE bs[2])
{
    // change to little endian
    if( IsBigEndian()){
        SwapEndian(&st, 1);
    }

    bs[0] = ((BYTE*)&st)[0];
    bs[1] = ((BYTE*)&st)[1];
}

short LittleEndianBytes2Short(BYTE low, BYTE high)
{
    short st = 0;
    ((BYTE*)&st)[0] = low;
    ((BYTE*)&st)[1] = high;

    if(IsBigEndian()){
        SwapEndian(&st,1);
    }

    return st;
}

SilkCodec::SilkCodec(const Napi::CallbackInfo& info): Napi::ObjectWrap<SilkCodec>(info) 
{
    std::thread::id tid = std::this_thread::get_id();
    std::cout<<"["<<tid<<"]SilkCodec::SilkCodec"<<std::endl;
    Napi::Env env = info.Env();

	if(info.Length() < 4 ){
		Napi::Error::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return;
	}

	if(!info[0].IsString() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber()){
		Napi::TypeError::New(env, "invalid type of arguments").ThrowAsJavaScriptException();
		return;
	}

	std::string type = (std::string) info[0].ToString();
  	int sample_rate = info[1].As<Napi::Number>().Int32Value();
    int sample_bits = info[2].As<Napi::Number>().Int32Value();
  	int channel = info[3].As<Napi::Number>().Int32Value();

    std::cout<<"["<<tid<<"]SilkCodec::SilkCodec"
         << ", type:"<<type
         << ", sample_rate:"<<sample_rate
         << ", sample_bits:"<<sample_bits
         << ", channel:"<<channel
         <<std::endl;

	//check sample_rate
	if(sample_rate != 8000 && sample_rate != 16000 && sample_rate != 24000 && 
		sample_rate != 32000 && sample_rate != 44100 && sample_rate != 48000){
		Napi::TypeError::New(env, "invalid sample rate, only support 8000/16000/24000/32000/44100/48000").ThrowAsJavaScriptException();
		return;
	}
    m_sample_rate = sample_rate;

    // check sample_bit
	if(sample_bits != 8 && sample_bits != 16 && sample_bits != 24 && sample_bits != 32){
		Napi::TypeError::New(env, "invalid sample bit, only support 8/16/24/32").ThrowAsJavaScriptException();
		return;
	}
    m_sample_bits = sample_bits;

	// check channel
	if(channel != 1 && channel != 2){
		Napi::TypeError::New(env, "invalid channel number, only support 1/2").ThrowAsJavaScriptException();
		return;
	}
    m_channel = channel;

    // check type
    if(type == "encoder"){
        SKP_int32 encSizeBytes = 0;
	    int ret = SKP_Silk_SDK_Get_Encoder_Size(&encSizeBytes);
	    if (ret != 0) {
            std::cout<<"["<<tid<<"]SilkCodec::SilkCodec"
                     << ", SKP_Silk_SDK_Get_Encoder_Size failed, ret:"<<ret
                     <<std::endl;
            Napi::Error::New(env, "get silk encoder size failed").ThrowAsJavaScriptException();
		    return;
	    }

        m_pEncoder = malloc(encSizeBytes);
        SKP_SILK_SDK_EncControlStruct encStatus;
        ret = SKP_Silk_SDK_InitEncoder(m_pEncoder, &encStatus);
        if (ret != 0) {
            std::cout<<"["<<tid<<"]SilkCodec::SilkCodec"
                     << ", SKP_Silk_SDK_InitEncoder failed, ret:"<<ret
                     <<std::endl; 
            Napi::Error::New(env, "init silk encoder failed").ThrowAsJavaScriptException();
            return;
        } 
    }

    else if (type == "decoder"){
        int decSizeBytes = 0;
        int ret = SKP_Silk_SDK_Get_Decoder_Size(&decSizeBytes);
        if( ret ) {
            std::cout<<"["<<tid<<"]SilkCodec::SilkCodec"
                     << ", SKP_Silk_SDK_Get_Decoder_Size failed, ret:"<<ret
                     <<std::endl; 
            Napi::Error::New(env, "get silk decoder size failed").ThrowAsJavaScriptException();
            return;
        }

        m_pDecoder = malloc(decSizeBytes);
        ret = SKP_Silk_SDK_InitDecoder(m_pDecoder);
        if( ret ) {
            std::cout<<"["<<tid<<"]SilkCodec::SilkCodec"
                     << ", SKP_Silk_InitDecoder failed, ret:"<<ret
                     <<std::endl; 
            Napi::Error::New(env, "init silk decoder failed").ThrowAsJavaScriptException();
            return;
        }
    }

    else{
        Napi::Error::New(env, "invalid codec type").ThrowAsJavaScriptException();
        return;
    }
}

SilkCodec::~SilkCodec() {
    std::cout<<"["<<std::this_thread::get_id()<<"]SilkCodec::~SilkCodec"<<std::endl;

    if (m_pEncoder) {
		free(m_pEncoder);
		m_pEncoder = nullptr;
	}

    if (m_pDecoder) {
		free(m_pDecoder);
		m_pDecoder = nullptr;
	}
}

Napi::Object SilkCodec::Init(Napi::Env env, Napi::Object exports) {
	std::cout<<"["<<std::this_thread::get_id()<<"]SilkCodec::Init"<<std::endl;
	
    Napi::Function func = DefineClass(env, "SilkCodec", {
		InstanceMethod("encode", &SilkCodec::Encode),
		InstanceMethod("decode", &SilkCodec::Decode)
	});

	constructor = Napi::Persistent(func);
	constructor.SuppressDestruct();
	exports.Set("SilkCodec", func);
	return exports;
}

/* Encode: encoding pcm buffer to silk buffer
 * info[0]: pcm array buffer
 * return: silk array buffer, silk package size 20ms.  format: [len+data][len+data][...]
 */
Napi::Value SilkCodec::Encode(const Napi::CallbackInfo& info)
{
    std::thread::id tid = std::this_thread::get_id();
    std::cout<<"["<<tid<<"]SilkCodec::Encode"<<std::endl;
    Napi::Env env = info.Env();

    if (info.Length() < 1) {
        Napi::Error::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    if (!info[0].IsArrayBuffer()) {
        Napi::Error::New(info.Env(), "First Parameter must be an ArrayBuffer").ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    int bit_rate = 10000;
    if (info.Length() >= 2 && info[1].IsNumber()) {
        bit_rate = info[1].As<Napi::Number>().Int32Value();
        if(bit_rate < 100 || bit_rate > 100000){
            std::cout<<"["<<tid<<"]SilkCodec::Encode"
                     <<", invalid bit_rate: " << bit_rate
                     <<", using default value: 10000"
                     <<std::endl;
            bit_rate = 10000;
        }
    }

    Napi::ArrayBuffer buf = info[0].As<Napi::ArrayBuffer>();
    BYTE* p = reinterpret_cast<BYTE*>(buf.Data());
    std::vector<BYTE> pcm_in(p, p + buf.ByteLength() );

    // encode 20ms audio data each package
	int nBytesPer20ms = m_sample_rate * m_channel * m_sample_bits / 8 / 50;
    std::vector<BYTE> silk_out;
    int pkg_cnt = 0;
    while(pcm_in.size() >= size_t(nBytesPer20ms)) {
        pkg_cnt ++;
        EncodeInternal(pcm_in, silk_out, 20, bit_rate); // will append the encoded silk data into silk_buf
        pcm_in.erase(pcm_in.begin(), pcm_in.begin() + nBytesPer20ms);
    }

    std::cout<<"["<<tid<<"]SilkCodec::Encode"
            <<", input pcm size: " << buf.ByteLength()
            <<", output silk size: " << silk_out.size()
            <<", package count: " << pkg_cnt
            <<std::endl;
    return Napi::Buffer<BYTE>::Copy(env, silk_out.data(), silk_out.size());
}

// silk buf format: [len+data][len+data][...]
// len: package size. uint16 in little endian
// data: raw silk data of package
Napi::Value SilkCodec::Decode(const Napi::CallbackInfo& info)
{
    std::thread::id tid = std::this_thread::get_id();
    std::cout<<"["<<tid<<"]SilkCodec::Decode"<<std::endl;
    Napi::Env env = info.Env();

    if (info.Length() < 1) {
        Napi::Error::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    if (!info[0].IsArrayBuffer()) {
        Napi::Error::New(info.Env(), "First Parameter must be an ArrayBuffer").ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    Napi::ArrayBuffer buf = info[0].As<Napi::ArrayBuffer>();
    BYTE* p = reinterpret_cast<BYTE*>(buf.Data());
    std::vector<BYTE> silk_in(p, p + buf.ByteLength() );

    std::vector<BYTE> pcm_out;
    int pkg_cnt = 0;
    while(silk_in.size() > 2){
        short pkg_size = LittleEndianBytes2Short(silk_in[0], silk_in[1]);
        //printf("pkg_size: %d, silk_in[0]=%x, silk_in[1]=%x\n", pkg_size, silk_in[0], silk_in[1]);

        if(silk_in.size() < size_t(pkg_size + 2)){
            std::cout<<"["<<tid<<"]SilkCodec::Decode"
                     <<", remain not enough"
                     <<", remain: " << silk_in.size() - 2
                     <<", pkg_size: " << pkg_size
                     <<std::endl;
            break;
        }

        pkg_cnt ++;
        std::vector<BYTE> silk_package(silk_in.begin() + 2, silk_in.begin() + 2 + pkg_size);
        silk_in.erase(silk_in.begin(), silk_in.begin() + 2 + pkg_size);
        DecodeInternal(silk_package, pcm_out);
    }

    std::cout<<"["<<tid<<"]SilkCodec::Decode"
            <<", input silk size: " << buf.ByteLength()
            <<", output pcm size: " << pcm_out.size()
            <<", package count: " << pkg_cnt
            <<std::endl;

    return Napi::Buffer<BYTE>::Copy(env, pcm_out.data(), pcm_out.size());
}

bool SilkCodec::EncodeInternal(const std::vector<BYTE>& pcm_in, std::vector<BYTE>& silk_out, int duration_ms/* = 20*/, int bit_rate/* = 10000*/)
{
	if (nullptr == m_pEncoder) return false;

	SKP_SILK_SDK_EncControlStruct encControl;
	encControl.API_sampleRate = m_sample_rate;
	encControl.maxInternalSampleRate = m_sample_rate;
	encControl.packetSize = (duration_ms * m_sample_rate) / 1000;
	encControl.complexity = 2;
	encControl.packetLossPercentage = 0;
	encControl.useInBandFEC = 0;
	encControl.useDTX = 0;
	encControl.bitRate = bit_rate;

	// encode
	short nBytes = 2048;
	SKP_uint8 payload[2048];
	int ret = SKP_Silk_SDK_Encode(m_pEncoder, &encControl, (const short*)&pcm_in[0], encControl.packetSize, payload, &nBytes);
	if (ret) {
		printf("SKP_Silk_Encode failed, ret-%d\n", ret);
		return false;
	}

    BYTE bytes[2];
	Short2LittleEndianBytes(nBytes, bytes);
	silk_out.insert(silk_out.end(), &bytes[0], &bytes[2]);
	silk_out.insert(silk_out.end(), &payload[0], &payload[nBytes]);
    return true;
}

bool SilkCodec::DecodeInternal(const std::vector<BYTE>& silk_in, std::vector<BYTE>& pcm_out)
{
    SKP_SILK_SDK_DecControlStruct decControl;
    decControl.API_sampleRate = (int)m_sample_rate; // I: Output signal sampling rate in Hertz; 8000/12000/16000/24000
    
    int frames = 0;
    SKP_int16 out[ ( ( FRAME_LENGTH_MS * MAX_API_FS_KHZ ) << 1 ) * MAX_INPUT_FRAMES ] = {0};
    SKP_int16 *outPtr    = out;
    short     out_size = 0;
    short     len        = 0;
    do {
        /* Decode 20 ms */
        int ret = SKP_Silk_SDK_Decode(m_pDecoder, &decControl, 0, (unsigned char*) silk_in.data(), (int)silk_in.size(), outPtr, &len);
        if( ret ) {
            printf( "SKP_Silk_SDK_Decode returned %d\n", ret );
        }

        frames++;
        outPtr += len;
        out_size += len;
        if( frames > MAX_INPUT_FRAMES ) {
            /* Hack for corrupt stream that could generate too many frames */
            outPtr     = out;
            out_size = 0;
            frames     = 0;
            break;
        }
    /* Until last 20 ms frame of packet has been decoded */
    } while( decControl.moreInternalDecoderFrames);

    // using little endian
    if( IsBigEndian()){
        SwapEndian(out, out_size);
    }

    // append to pcm out
    BYTE* p = (BYTE*)out;
    pcm_out.insert(pcm_out.end(), p, p + out_size*2 );

    return true;
}