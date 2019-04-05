#ifndef AWS_CLIENT_CREDENTIAL_KEYS_H
#define AWS_CLIENT_CREDENTIAL_KEYS_H

#include <stdint.h>

/*
 * PEM-encoded client certificate.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----"
 */
#define keyCLIENT_CERTIFICATE_PEM \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDWTCCAkGgAwIBAgIUKHXWEu5StAMiUCEmcDE77/k1NBQwDQYJKoZIhvcNAQEL\n"\
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"\
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTE5MDMyNzA5MDIw\n"\
"NFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"\
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALTdv5SjjU0MFV5ayhlC\n"\
"GjwReQV8wNjIvbJXjt41F1M3BSgscMbb4xWZGn59ZBvFdN6f15BF+dAWRp+8Ej0O\n"\
"I7pAz5bcEic0ytreTSATnmOzVGI25J/Y+Y7IPHKT2Ku9G9zludayZhd2tkhM6tsE\n"\
"V8iWo507HOWxG0nAPSmCTmgSckcmpLUKdvVouL8+BxHgXdvYOXWiyTQxjm14nFSv\n"\
"qOnkOB+Pt9EGLK8avakjwM7TBlduTt2U6pwGOI3CKD8TEOiVQji8LZheax3GTuRz\n"\
"be7eqvnyk+E7XnycF6i9XaxSLeEYF0AC9scenWUwHothvAeCL0fxnZbhgPpSWdZp\n"\
"698CAwEAAaNgMF4wHwYDVR0jBBgwFoAUWC8hNVSvwwCjAxp/TTfDctn3i2IwHQYD\n"\
"VR0OBBYEFOKGb56cQ4s5kJ5Y/Kwh7Vr0b9iCMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"\
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQCLPPOrgVeNG8e/gB9I0CHZaIbG\n"\
"sN8XK+iyyP5EaE651YLRWkY+DyGs0ZCUU5wbHRHbUQRjiDMHbeZxiajeyi6K8Z+p\n"\
"TvDsOSpiCI2NxTNSs3gd50QoXb2bbCG0zZov1hKz8XwAN/4iZr2yChJ82nuzSXQu\n"\
"L0f83/zxoFhPaXfkW8KLLcN/BhJZFM+8DgHgMTaokLUVJ/4l1jGsJhSgwDADZCPy\n"\
"YzhF2Fuc19McQ6rp+7hw92axaoOqf90sxypwYtNhYmeT6ZalfS0K7mJ37hnUDq7L\n"\
"c4DIqLMM8tSvefppZAzl3pXk6WRv/B1qySqGI0PzcTKHsyKqnFYWxEA/QlYj\n"\
"-----END CERTIFICATE-----"

/*
 * PEM-encoded client private key.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN RSA PRIVATE KEY-----\n"\
 * "...base64 data...\n"\
 * "-----END RSA PRIVATE KEY-----"
 */
#define keyCLIENT_PRIVATE_KEY_PEM \
"-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEogIBAAKCAQEAtN2/lKONTQwVXlrKGUIaPBF5BXzA2Mi9sleO3jUXUzcFKCxw\n"\
"xtvjFZkafn1kG8V03p/XkEX50BZGn7wSPQ4jukDPltwSJzTK2t5NIBOeY7NUYjbk\n"\
"n9j5jsg8cpPYq70b3OW51rJmF3a2SEzq2wRXyJajnTsc5bEbScA9KYJOaBJyRyak\n"\
"tQp29Wi4vz4HEeBd29g5daLJNDGObXicVK+o6eQ4H4+30QYsrxq9qSPAztMGV25O\n"\
"3ZTqnAY4jcIoPxMQ6JVCOLwtmF5rHcZO5HNt7t6q+fKT4TtefJwXqL1drFIt4RgX\n"\
"QAL2xx6dZTAei2G8B4IvR/GdluGA+lJZ1mnr3wIDAQABAoIBACNSdzrRIiRc6sNt\n"\
"mbCLH1KB2w++SG9N7PYqcWVmGvMOeLxayX+cq+Go7+qqFGX0Dxn3P4MWl24TG6HQ\n"\
"egNggou6YpBE1GrBvag25/sOuc0g1acEr2ukVUCUTHHX++W2Xf2rA7sxRgzpCzpd\n"\
"gzRYycgL21I2nDMjfn/NwmQb5bIfK5/8Bs9kBIwpxxs5pqPOEYowCfLf8acMMQYi\n"\
"LT0zXUYwjt24aVpu8bC+OQIdPl5QwiHdhnvMy0EpdZlfoSiobwS5yjYJD0TvH6RQ\n"\
"IWFKzOj1wf08P6vhuCmdghpsfRlLJ3B3b2tfXeuTiakL62HAxRHnw9t8iEOaje0B\n"\
"QuDNTwECgYEA7rodSaV/+NEiDnChtpT5eFHqVyNhcPoFz2lR9klRS6lyfwLXgDN0\n"\
"iYVV5zQ1HF5zz9YIiAn38J/U9FhJ5FszfubwhnzOREC/BMMvMgqBFqOlz/XCGL0f\n"\
"RQ/vHuaJfx3U905ZrafH4xxyn1vajgh+QgIwQyFHFjF3siDVAqZKT4ECgYEAwfPk\n"\
"PKZlbltalNhn/jZcvSumect8o4NmVE0nFMkXY6Qip+KhJ2rWx8CVGVXTIWrjkwUs\n"\
"uai5auoyXw3m8XU7mp+4PHKZI4bmA4mUGT4cYVbjv/lxm6QXib8g8keKi7HJbKo/\n"\
"hlxZsYAKEy+5w4WV9EG/furfqHS89sLxsSdb618CgYB7oJBYOkfKf+smFTf5yK5k\n"\
"m+9CjwUAL8pgfgc/Bvr9ttOfaMZXEs6QBSfWYtf6SAOrpwimApuO1ga/PxWNF5nU\n"\
"Zx46V1muSOVjPv0q6fut0LOmvXt1ukL+TeEkXHjqBnXqWH3ii1fdijblxfipw0Y3\n"\
"QtDhgtNAb/+vlIcbJyimgQKBgFlHw9CHaDmco93QoE5NB/OsnD2EhiNe4UP9H/hZ\n"\
"eQLNE4bks/pJHAVniTrYXxAK/Cc5QpVULcYheH55D84mgQF0dIKe3g+UkV0ff6T/\n"\
"CmFkdgJw+PMEXjFyYeAlPol/lZEH7aYT7NXgcsLSPVjbrWN6wIPT6pDI7BNLOaaq\n"\
"z41RAoGAa5RUWV8Gy2Ope4H5anJF/TFV36ha1MuAocHyYc0bEp8n4vaKt8LKkzjP\n"\
"qwzohSRTiBHWBGmU1kgDqPf6H3iXTcNoA6c7v+uCQY9y9Tv6M0l0duGxTqyDxvaB\n"\
"IWDOqKAXxtT7f4e0SaJFsD9tNfwZ62gzsuyvA71KcLwBIewYOLU=\n"\
"-----END RSA PRIVATE KEY-----"

/*
 * PEM-encoded Just-in-Time Registration (JITR) certificate (optional).
 *
 * If used, must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----"
 */
#define keyJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM  NULL

/* The constants above are set to const char * pointers defined in aws_dev_mode_key_provisioning.c,
 * and externed here for use in C files.  NOTE!  THIS IS DONE FOR CONVENIENCE
 * DURING AN EVALUATION PHASE AND IS NOT GOOD PRACTICE FOR PRODUCTION SYSTEMS
 * WHICH MUST STORE KEYS SECURELY. */
extern const char clientcredentialCLIENT_CERTIFICATE_PEM[];
extern const char* clientcredentialJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM;
extern const char clientcredentialCLIENT_PRIVATE_KEY_PEM[];
extern const uint32_t clientcredentialCLIENT_CERTIFICATE_LENGTH;
extern const uint32_t clientcredentialCLIENT_PRIVATE_KEY_LENGTH;

#endif /* AWS_CLIENT_CREDENTIAL_KEYS_H */
