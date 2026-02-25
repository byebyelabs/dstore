# dstore

### How to run:

- Create `.env` file with variable `OPENSSL_PATH`
- `make` to build project
- `make spawn` to spawn 3 storage nodes and a balancer
- In a new terminal: `make test` to run client tests
- `make kill` to kill background processes (storage nodes)

### Formats:

- `SET` message format: `<KEY_HASH><VAL_LEN>#<VAL>`
- `GET` message format: `<KEY_HASH>`
  - Needs to return to client directly potentially
- `DEL` message format: `<KEY_HASH>`
