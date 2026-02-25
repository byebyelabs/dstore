MAP = {}

# ensure all log entries are correct
with open("test_log.txt", "r") as test_logs:
    for i, test in enumerate(test_logs):
        ts, tst = test.split(" | ")
        typ, response = tst.split(": ")
        key, val = response.split(" -> ")

        key = key.strip()
        val = val.strip()

        if typ == "SET":
            MAP[key] = val

        elif typ == "GET":
            should_be = MAP.get(key, "")
            assert (
                should_be == val
            ), f"line {i}: GET {key} should be {should_be} but got {val}"

        else:
            assert False, "INVALID LOG ENTRY"

print("all SETs and GETs are consistent")

# print summary stats for client
print("For client: ")
for op in ["set", "get"]:
    with open(f"client_{op}_log.txt", "r") as logs:
        ttime = 0
        count = 0
        for line in logs:
            start, end = line.split()
            ttime += int(end) - int(start)
            count += 1

    print(f"  avg latency for {op}: {ttime / count}μs over {count} operations")


# print summary stats for storage node
print("For storage nodes: ")
for op in ["set", "get"]:
    with open(f"storage_node_{op}_log.txt", "r") as logs:
        ttime = 0
        count = 0
        for line in logs:
            start, end = line.split()
            ttime += int(end) - int(start)
            count += 1

    print(f"  avg latency for {op}: {ttime / count}μs over {count} operations")
