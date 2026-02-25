# env must define OPENSSL_PATH
include .env

CC := clang
CFLAGS := -g -Wall -Werror -fsanitize=address -Wno-unused-function -Wno-unused-variable -I. -I./headers -I${OPENSSL_PATH}/include
# you should have libcrypto from openssl.
# we need it for md5 hashing.
LIBS := -lcrypto -L${OPENSSL_PATH}/lib
# default number of tests
NUM_TESTS := 1000

all:
	@$(MAKE) balancer
	@$(MAKE) storage
	@$(MAKE) client

balancer: server/balancer.c workers/balancer.c util.c message.c
	@mkdir -p out
	${CC} ${CFLAGS} -o out/balancer server/balancer.c workers/balancer.c util.c message.c ${LIBS} -lpthread

storage: server/storage.c util.c message.c
	@mkdir -p out
	${CC} ${CFLAGS} -o out/storage server/storage.c util.c message.c ${LIBS} -lpthread

client: server/client.c message.c util.c
	@mkdir -p out
	${CC} ${CFLAGS} -o out/client test.c server/client.c message.c util.c ${LIBS}

spawn:
	@make
	@rm -f .storage_ports
	@touch .storage_ports
	@./out/storage &
	@./out/storage &
	@./out/storage &
	@sleep 1
	./out/balancer $$(cat .storage_ports)

# Citation: https://stackoverflow.com/a/8987063
kill:
	pkill -f out/storage
	rm -f .storage_ports


# need to || true so removing of stale logs happens
test:
	@make spawn &
	@sleep 2
	@make client
	./out/client ${NUM_TESTS} 67
	@python3 assert_test_correctness.py || true
	@rm -f *_log.txt
	@make kill
