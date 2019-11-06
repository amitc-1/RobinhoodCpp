#include <iostream>
#include <string>
#include <cstring>
//for GenerateDeviceToken
#include <random>
#include <sstream>
#include <vector>
//for generateMFACode
#include <chrono>
#include <time.h> 
#include "sha1.h"
#include <cstdint>
#include "authentication.h"

using namespace std;

std::string RobinhoodAuthentication::auth_token;
std::string RobinhoodAuthentication::refresh_token;

const int VERIFICATION_CODE_MODULUS = (1000 * 1000);
const int BITS_PER_BASE32_CHAR = 5;

extern "C" int base32_decode(const uint8_t *encoded, uint8_t *result, int bufSize);
extern "C" void hmac_sha1(const uint8_t *key, int keyLength,
	const uint8_t *data, int dataLength,
	uint8_t *result, int resultLength);

string RobinhoodAuthentication::generateDeviceToken() {
	std::default_random_engine dre;
	std::uniform_real_distribution<float> dr(0.0, 1.0);

	vector<int> rands = vector<int>();
	for (int i = 0; i < 16; i++)
	{
		auto r = dr(dre);  //rng.Nextfloat();
		float rand = static_cast<float>(4294967296.0 * r);
		rands.push_back(((int)((unsigned int)rand >> ((3 & i) << 3))) & 255);
	}

	vector<string> hex = vector<string>();
	for (int i = 0; i < 256; ++i)
	{
		std::stringstream stream;
		stream << std::hex << (i + 256);
		hex.push_back(stream.str().substr(1));
	}

	string id = "";
	for (int i = 0; i < 16; i++)
	{
		id += hex[rands[i]];

		if (i == 3 || i == 5 || i == 7 || i == 9)
		{
			id += "-";
		}
	}
	cout << "generateDeviceToken(): id=" << id << endl;
	return id;
}


int RobinhoodAuthentication::generateMFACode(const char *key, int step_size) {
	uint8_t challenge[8];
	unsigned long tm = static_cast<unsigned long> (time(NULL) / step_size);
	for (int i = 8; i--; tm >>= 8) {
		challenge[i] = static_cast<uint8_t>(tm);
	}

	// Estimated number of bytes needed to represent the decoded secret. Because
	// of white-space and separators, this is an upper bound of the real number,
	// which we later get as a return-value from base32_decode()
	int secretLen = (strlen(key) + 7) / 8 * BITS_PER_BASE32_CHAR;

	// Sanity check, that our secret will fixed into a reasonably-sized static
	// array.
	if (secretLen <= 0 || secretLen > 100) {
		return -1;
	}

	// Decode secret from Base32 to a binary representation, and check that we
	// have at least one byte's worth of secret data.
	uint8_t secret[100];
	if ((secretLen = base32_decode((const uint8_t *)key, secret, secretLen)) < 1) {
		return -1;
	}

	// Compute the HMAC_SHA1 of the secret and the challenge.
	uint8_t hash[SHA1_DIGEST_LENGTH];
	hmac_sha1(secret, secretLen, challenge, 8, hash, SHA1_DIGEST_LENGTH);

	// Pick the offset where to sample our hash value for the actual verification
	// code.
	const int offset = hash[SHA1_DIGEST_LENGTH - 1] & 0xF;

	// Compute the truncated hash in a byte-order independent loop.
	unsigned int truncatedHash = 0;
	for (int i = 0; i < 4; ++i) {
		truncatedHash <<= 8;
		truncatedHash |= hash[offset + i];
	}

	// Truncate to a smaller number of digits.
	truncatedHash &= 0x7FFFFFFF;
	truncatedHash %= VERIFICATION_CODE_MODULUS;

	cout << "generateMFACode(): 2fa code:" << truncatedHash << endl;
	return truncatedHash;
}
