CC := clang
OPENSSL_PATH := /opt/homebrew/opt/openssl
CFLAGS := -g -Wall -Werror -Wno-unused-function -Wno-unused-variable -I./headers -I${OPENSSL_PATH}/include
# you should have libcrypto from openssl.
# we need it for md5 hashing.
LIBS := -lcrypto -L${OPENSSL_PATH}/lib

all:
	${storage}
	${test}

storage: storage.c
	@mkdir -p out
	${CC} ${CFLAGS} -o out/storage storage.c util.c ${LIBS}

test: test.c storage.c
	@mkdir -p out
	${CC} ${CFLAGS} -o out/test test.c storage.c util.c ${LIBS}