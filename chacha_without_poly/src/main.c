/*
 * Copyright (c) 2019-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#endif

#define APP_SUCCESS (0)
#define APP_ERROR (-1)
#define APP_ERROR_MESSAGE "Example exited with error!"

#define PRINT_HEX(p_label, p_text, len)                           \
	({                                                        \
		LOG_INF("---- %s (len: %u): ----", p_label, len); \
		LOG_HEXDUMP_INF(p_text, len, "Content:");         \
		LOG_INF("---- %s end  ----", p_label);            \
	})

LOG_MODULE_REGISTER(chachapoly, LOG_LEVEL_DBG);

static const uint8_t chacha20_testPlaintext[] = {
    0x4c, 0x61, 0x64, 0x69, 0x65, 0x73, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x47,
    0x65, 0x6e, 0x74, 0x6c, 0x65, 0x6d, 0x65, 0x6e, 0x20, 0x6f, 0x66, 0x20,
    0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x20, 0x6f, 0x66,
    0x20, 0x27, 0x39, 0x39, 0x3a, 0x20, 0x49, 0x66, 0x20, 0x49, 0x20, 0x63,
    0x6f, 0x75, 0x6c, 0x64, 0x20, 0x6f, 0x66, 0x66, 0x65, 0x72, 0x20, 0x79,
    0x6f, 0x75, 0x20, 0x6f, 0x6e, 0x6c, 0x79, 0x20, 0x6f, 0x6e, 0x65, 0x20,
    0x74, 0x69, 0x70, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x74, 0x68, 0x65, 0x20,
    0x66, 0x75, 0x74, 0x75, 0x72, 0x65, 0x2c, 0x20, 0x73, 0x75, 0x6e, 0x73,
    0x63, 0x72, 0x65, 0x65, 0x6e, 0x20, 0x77, 0x6f, 0x75, 0x6c, 0x64, 0x20,
    0x62, 0x65, 0x20, 0x69, 0x74, 0x2e};

/* To hold intermediate results in both Chacha20 and Chacha20-Poly1305 */
static uint8_t chacha20_testCiphertext[sizeof(chacha20_testPlaintext)] = {0};
static uint8_t chacha20_testDecryptedtext[sizeof(chacha20_testPlaintext)] = {0};

static const uint8_t chacha20_testKey[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

static const uint8_t chacha20_testNonce[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x4a,
    0x00, 0x00, 0x00, 0x00};

/* The initial counter of the Chacha20 RFC7539 test vectors is 1, while the PSA
 * APIs assume it to be zero. This means that this expected ciphertext is not
 * the same as the one presented in the RFC
 */
static const uint8_t chacha20_testCiphertext_expected[] = {
    0xe3, 0x64, 0x7a, 0x29, 0xde, 0xd3, 0x15, 0x28, 0xef, 0x56, 0xba, 0xc7,
    0x0f, 0x7a, 0x7a, 0xc3, 0xb7, 0x35, 0xc7, 0x44, 0x4d, 0xa4, 0x2d, 0x99,
    0x82, 0x3e, 0xf9, 0x93, 0x8c, 0x8e, 0xbf, 0xdc, 0xf0, 0x5b, 0xb7, 0x1a,
    0x82, 0x2c, 0x62, 0x98, 0x1a, 0xa1, 0xea, 0x60, 0x8f, 0x47, 0x93, 0x3f,
    0x2e, 0xd7, 0x55, 0xb6, 0x2d, 0x93, 0x12, 0xae, 0x72, 0x03, 0x76, 0x74,
    0xf3, 0xe9, 0x3e, 0x24, 0x4c, 0x23, 0x28, 0xd3, 0x2f, 0x75, 0xbc, 0xc1,
    0x5b, 0xb7, 0x57, 0x4f, 0xde, 0x0c, 0x6f, 0xcd, 0xf8, 0x7b, 0x7a, 0xa2,
    0x5b, 0x59, 0x72, 0x97, 0x0c, 0x2a, 0xe6, 0xcc, 0xed, 0x86, 0xa1, 0x0b,
    0xe9, 0x49, 0x6f, 0xc6, 0x1c, 0x40, 0x7d, 0xfd, 0xc0, 0x15, 0x10, 0xed,
    0x8f, 0x4e, 0xb3, 0x5d, 0x0d, 0x62};

int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS)
		return APP_ERROR;

	return APP_SUCCESS;
}

static inline int compare_buffers(const uint8_t *p1,
				  const uint8_t *p2,
				  size_t comp_size)
{
	return memcmp(p1, p2, comp_size);
}

int psa_cipher_rfc7539_test(void)
{
	psa_cipher_operation_t handle = psa_cipher_operation_init();
	psa_cipher_operation_t handle_dec = psa_cipher_operation_init();
	psa_status_t status = PSA_SUCCESS;
	psa_key_handle_t key_handle = 0;
	psa_key_attributes_t key_attributes = psa_key_attributes_init();
	const psa_algorithm_t alg = PSA_ALG_STREAM_CIPHER;
	bool bAbortDecryption = false;
	/* Variables required during multipart update */
	size_t data_left = sizeof(chacha20_testPlaintext);
	size_t lengths[] = {42, 24, 48};
	size_t start_idx = 0;
	size_t output_length = 0;
	size_t total_output_length = 0;
	int comp_result;
	int ret = APP_ERROR;

	/* Setup the key policy */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_STREAM_CIPHER);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_CHACHA20);
	psa_set_key_bits(&key_attributes, 256);

    	status = psa_import_key(&key_attributes, chacha20_testKey,
                        	sizeof(chacha20_testKey), &key_handle);
	if (status != PSA_SUCCESS)
	{
		LOG_INF("Error importing a key");
		return APP_ERROR;
	}

	/* Setup the encryption object */
	status = psa_cipher_encrypt_setup(&handle, key_handle, alg);
	if (status != PSA_SUCCESS)
	{
		LOG_INF("Encryption setup shouldn't fail");
		goto destroy_key;
	}

	/* Set the IV */
	status = psa_cipher_set_iv(&handle,
				   chacha20_testNonce, sizeof(chacha20_testNonce));
	if (status != PSA_SUCCESS)
	{
		LOG_INF("Error setting the IV on the cipher operation object");
		goto abort;
	}

	for (int i = 0; i < sizeof(lengths) / sizeof(size_t); i++)
	{
		/* Encrypt one chunk of information */
		status = psa_cipher_update(
		    &handle,
		    &chacha20_testPlaintext[start_idx],
		    lengths[i],
		    &chacha20_testCiphertext[total_output_length],
		    sizeof(chacha20_testCiphertext) - total_output_length,
		    &output_length);

		if (status != PSA_SUCCESS)
		{
			LOG_INF("Error encrypting one chunk of information");
			goto abort;
		}

		if (output_length != lengths[i])
		{
			LOG_INF("Expected encrypted length is different from expected");
			goto abort;
		}

		data_left -= lengths[i];
		total_output_length += output_length;

		start_idx += lengths[i];
	}

	/* Finalise the cipher operation */
	status = psa_cipher_finish(
	    &handle,
	    &chacha20_testCiphertext[total_output_length],
	    sizeof(chacha20_testCiphertext) - total_output_length,
	    &output_length);

	if (status != PSA_SUCCESS)
	{
		LOG_INF("Error finalising the cipher operation");
		goto abort;
	}

	if (output_length != 0)
	{
		LOG_INF("Un-padded mode final output length unexpected");
		goto abort;
	}

	/* Add the last output produced, it might be encrypted padding */
	total_output_length += output_length;

	/* Compare encrypted data produced with single-shot and multipart APIs */
	comp_result = compare_buffers(chacha20_testCiphertext_expected,
				      chacha20_testCiphertext,
				      total_output_length);
	if (comp_result != 0)
	{
		LOG_INF("Single-shot crypt doesn't match with multipart crypt");
		goto destroy_key;
	}

	/* Setup the decryption object */
	status = psa_cipher_decrypt_setup(&handle_dec, key_handle, alg);
	if (status != PSA_SUCCESS)
	{
		LOG_INF("Error setting up cipher operation object");
		goto destroy_key;
	}

	/* From now on, in case of failure we want to abort the decryption op */
	bAbortDecryption = true;

	/* Set the IV for decryption */
	status = psa_cipher_set_iv(&handle_dec,
				   chacha20_testNonce, sizeof(chacha20_testNonce));
	if (status != PSA_SUCCESS)
	{
		LOG_INF("Error setting the IV for decryption");
		goto abort;
	}

	/* Decrypt - total_output_length considers encrypted padding */
	data_left = total_output_length;
	/* Update in different chunks of plainText */
	lengths[0] = 14;
	lengths[1] = 70;
	lengths[2] = 30;
	start_idx = 0;
	output_length = 0;
	total_output_length = 0;
	for (int i = 0; i < sizeof(lengths) / sizeof(size_t); i++)
	{
		status = psa_cipher_update(
		    &handle_dec,
		    &chacha20_testCiphertext[start_idx],
		    lengths[i],
		    &chacha20_testDecryptedtext[total_output_length],
		    sizeof(chacha20_testDecryptedtext) - total_output_length,
		    &output_length);

		if (status != PSA_SUCCESS)
		{
			LOG_INF("Error decrypting one chunk of information");
			goto abort;
		}

		if (output_length != lengths[i])
		{
			LOG_INF("Expected encrypted length is different from expected");
			goto abort;
		}

		data_left -= lengths[i];
		total_output_length += output_length;

		start_idx += lengths[i];
	}

	/* Finalise the cipher operation for decryption */
	status = psa_cipher_finish(
	    &handle_dec,
	    &chacha20_testDecryptedtext[total_output_length],
	    sizeof(chacha20_testDecryptedtext) - total_output_length,
	    &output_length);

	if (status != PSA_SUCCESS)
	{
		LOG_INF("Error finalising the cipher operation");
		goto abort;
	}

	/* Finalize the count of output which has been produced */
	total_output_length += output_length;

	/* Check that the decrypted length is equal to the original length */
	if (total_output_length != sizeof(chacha20_testPlaintext))
	{
		LOG_INF("After finalising, unexpected decrypted length");
		goto destroy_key;
	}

	/* Check that the plain text matches the decrypted data */
	comp_result = compare_buffers(chacha20_testPlaintext,
				      chacha20_testDecryptedtext,
				      sizeof(chacha20_testPlaintext));
	if (comp_result != 0)
	{
		LOG_INF("Decrypted data doesn't match with plain text");
	}
	else
	{
		ret = APP_SUCCESS;
	}

	/* Go directly to the destroy_key label at this point */
	goto destroy_key;

abort:
	/* Abort the operation */
	status = bAbortDecryption ? psa_cipher_abort(&handle_dec) : psa_cipher_abort(&handle);
	if (status != PSA_SUCCESS)
	{
		LOG_INF("Error aborting the operation");
	}
destroy_key:
	/* Destroy the key */
	status = psa_destroy_key(key_handle);
	if (status != PSA_SUCCESS)
	{
		LOG_INF("Error destroying a key");
	}

	return ret;
}

int main(void)
{
	int status;

	LOG_INF("Starting Chacha example...");
	status = crypto_init();
	if (status != APP_SUCCESS)
	{
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = psa_cipher_rfc7539_test();
	if (status != APP_SUCCESS)
	{
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	PRINT_HEX("Plain text", chacha20_testPlaintext, sizeof(chacha20_testPlaintext));
	PRINT_HEX("Cipher text", chacha20_testCiphertext, sizeof(chacha20_testCiphertext));
	PRINT_HEX("Decrypted text", chacha20_testDecryptedtext, sizeof(chacha20_testDecryptedtext));

	LOG_INF("Chacha example completed successfully.");

	return APP_SUCCESS;
}
