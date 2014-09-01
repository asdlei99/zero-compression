#include "zero_compression.h"
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static void write_ff(const uint8_t* src, uint8_t* dst, int32_t n){
	dst[0] = 0xff;
	dst[1] = n - 1;
	memcpy(dst + 2, src, n * 8);
}

static int32_t pack_seg(const uint8_t* src, uint8_t* buffer, int32_t sz, int32_t n)
{
	uint8_t header = 0;
	int32_t notzero = 0;
	int32_t i;
	uint8_t* obuffer = buffer;

	/*header��ʶλ*/
	++ buffer;
	-- sz;

	if(sz <= 0)
		obuffer = NULL;

	/*����8�ֽ�bitmap����*/
	for(i = 0; i < 8; i++){
		if(src[i] != 0){
			notzero ++;
			header |= 1 << i;
			if(sz > 0){
				*buffer = src[i];
				buffer ++;
				sz --;
			}
		}
	}

	if((notzero == 7 || notzero == 6) && n > 0) /*�����6�����ϵ�Ԫ��Ϊ0��ֱ�Ӻϲ���ǰ��FF�����У��������Խ�ʡ1���ֽڿռ�*/
		notzero = 8;

	/*��Ϊ0Ƭ��*/
	if(notzero == 8){
		if(n > 0)
			return 8;
		else  /*��һ��ȫ��Ϊ0��8�ֽ�Ƭ��*/
			return 10;
	}

	/*���ñ�ʶλ*/
	if(obuffer != NULL) 
		*obuffer = header;

	return notzero + 1;
}

int32_t proto_pack(const void* src, int32_t src_size, void* dst, int32_t dst_size)
{
	uint8_t tmp[8];
	int32_t i, ff_n = 0, size = 0;

	const int8_t* ff_srcstart = NULL;
	uint8_t* ff_deststart = NULL;
	const uint8_t* src_p = src;
	uint8_t* buffer = dst;

	for(i = 0; i < src_size; i += 8){ 
		/*�ж��Ƿ�Ҫ0����*/
		int32_t n = 0, j = 0;
		int32_t padding = i + 8 - src_size;
		if(padding > 0){
			memcpy(tmp, src_p, 8 - padding);
			for(j = 0; j < padding; j ++)
				tmp[7 - j] = 0;

			src_p = tmp;
		}

		/*����8�ֽ�Ϊ��λ��Ƭ�δ��*/
		n = pack_seg(src_p, buffer, dst_size, ff_n);
		dst_size -= n;

		/*��һ������8���ֽڶ���Ϊ0,��¼��ʼλ��*/
		if(n == 10){
			ff_srcstart = src_p;
			ff_deststart = buffer;
			ff_n = 1;
		}
		else if(n == 8 && ff_n > 0){/*������8�ֽڲ�Ϊ0*/
			++ ff_n;
			if(ff_n >= 256){ /*���256��������Ϊ0��8�ֽ�*/
				write_ff(ff_srcstart, ff_deststart, 256);
				ff_n = 0;
			}
		}
		else{
			if(ff_n > 0)
				write_ff(ff_srcstart, ff_deststart, ff_n);

			ff_n = 0;
		}

		/*������һ��8�ֽ��ж�*/
		src_p += 8;
		buffer += n;
		size += n;
	}

	if(ff_n > 0 && dst_size >= 0)
		write_ff(ff_srcstart, ff_deststart, ff_n);

	return size;
}

int32_t proto_unpack(const void* srcv, int32_t src_size, void* dstv, int32_t dst_size)
{
	uint8_t header = 0;
	const uint8_t* src = srcv;
	uint8_t* buffer = dstv;
	int32_t size = 0, n = 0;

	while(src_size > 0){
		header = src[0];
		-- src_size;
		++ src;
		if(header == 0xff){ /*������8�ֽ�Ƭ�β�Ϊ0*/
			if(src_size < 0)
				return -1;

			/*����Ƭ������*/
			n = (src[0] + 1) * 8;
			if(src_size < n + 1)
				return -1;

			src_size -= n + 1;
			++ src;

			/*�������ݿ���*/
			if(dst_size >= n)
				memcpy(buffer, src, n);

			dst_size -= n;
			buffer += n;
			src += n;
			size += n;
		}
		else {
			int32_t i;
			for (i = 0; i < 8; i++) { /*�����0��ֵ*/
				int nz = (header >> i) & 1;
				if(nz){
					if(src_size < 0)
						return -1;

					if(dst_size > 0){
						*buffer = *src;
						-- dst_size;
						++ buffer;
					}
					++ src;
					-- src_size;
				} 
				else {/*0���*/
					if (dst_size > 0){
						*buffer = 0;
						-- dst_size;
						++ buffer;
					}
				}

				++ size;
			}
		}
	}

	return size;
}

#ifdef __cplusplus
}
#endif


