#ifndef _SILK_CODEC_H
#define _SILK_CODEC_H

#include <napi.h>
#include <string>
#include <vector>

typedef unsigned char BYTE;

class SilkCodec : public Napi::ObjectWrap<SilkCodec> {
	public:
		static Napi::Object Init(Napi::Env env, Napi::Object exports);
		explicit SilkCodec(const Napi::CallbackInfo& info);
		~SilkCodec();
	private:
		static Napi::FunctionReference constructor;
        Napi::Value Encode(const Napi::CallbackInfo& info);
        Napi::Value Decode(const Napi::CallbackInfo& info);

        bool EncodeInternal(const std::vector<BYTE>& pcm_in, std::vector<BYTE>& silk_out, int duration_ms = 20, int bit_rate = 10000);
        bool DecodeInternal(const std::vector<BYTE>& silk_in, std::vector<BYTE>& pcm_out);

        void* m_pEncoder = nullptr;
        void* m_pDecoder = nullptr;
        int m_sample_rate = 0;
        int m_sample_bits = 0;
        int m_channel = 0;
};

#endif
