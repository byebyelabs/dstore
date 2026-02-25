# dstore

DStore: Distributed Key-Value Storage System.

## System Design

Our implementation has three main parts: the client, the server, and storage nodes.

The client is the one setting and getting key-value pairs.
The storage nodes are storing the key-value pairs.
Meanwhile, the server communicates with the client and storage nodes.

Storage nodes "join" the system by communicating to client. Once a storage node joins, it is responsible for storing some of all possible keys. We do this using a naive consistent-hashing implementation. Our implementation relies on the assumption that no nodes leave or join the system once the client starts storing key-value pairs. This is because we have not yet implemented functionality to transfer ownership of keys (required for nodes to join/leave while maintaining data consistency).

When a client sets a key-value pair, it communicates with the server (by sending a message) the key-value pair. Then, the server hashes the key, and using that hash, finds the storage node that "owns" (is responsible for storing data in) that key-space. Then, the server communicates with that storage node directly (by sending a message) the key-value pair. The storage node now stores the key-value pair.

When a client attempts to get a key-value pair, it communicates with the server the key. Then, the server hashes the key, and using that hash, finds the storage node that "owns" that key-space. Then, the server communicates with that storage node, requesting the value associated with the key. The storage node fetches the value (stored in a linked list) and reports it back to the server. The server then reports the value back to the client (if it exists).

The server assignes storage nodes to key-space by using a naive consistent-hashing approach. It assigns each storage node a hash (using its host:port name), and organizes that storage node in a circular doubly linked list of other storage nodes that are ordered by their hashes. Then, to assing keys to storage nodes, the server hashes the key, and finds the successor storage node from that key-hash on the circular linked list. As long as >= 1 storage nodes are in the system, keys will have a home.

Since we always directly store the key-value pair to the correct storage node (not a lazy operation), we consider our system always consistent. We are not creating any replicas of data, so it always lives in its original storage node.

### Run Program and Tests:

- Create `.env` file with variable `OPENSSL_PATH`
- `make` to build project
- `make spawn` to spawn 3 storage nodes and a balancer
- **Testing**: In a new terminal: `make test` to run client tests; takes about 2 minutes to run by default!
- `make kill` to kill background processes (storage nodes)

## Testing:

- See above to run our progam and test
- The test terminal will report average latency for client and storage nodes
- To configure the test,

### Client Code:

- See our client header at `headers/client.h` for description on the following:
  - `void dset(char *key, char *value)`
  - `void dget(char *key, char *value_out)`

### Formats:

- `SET` message format: `<KEY_HASH><VAL_LEN>#<VAL>`
- `GET` message format: `<KEY_HASH>`
  - Needs to return to client directly potentially
- `DEL` message format: `<KEY_HASH>`
