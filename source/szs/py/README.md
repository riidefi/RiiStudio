# WIP python bindings

```py
def main():
    # 1. Get the version of the library
    try:
        version = RIISZS.get_version()
        print(f"{GREEN}RIISZS Library Version: {version}{RESET}")
    except RIISZSError as e:
        print(f"{RED}Error fetching version: {e}{RESET}")

    # 2. Check if a given byte stream is compressed
    sample_data = b"This is a sample data for sample demonstration purpose this is."
    is_compressed_flag = RIISZS.is_compressed(sample_data)
    print(f"{YELLOW}Is the sample data compressed? {'Yes' if is_compressed_flag else 'No'}{RESET}")

    # 3. Encode and decode the data
    try:
        print(f"{YELLOW}Trying to encode...{RESET}")
        encoded_data = RIISZS.encode(sample_data, EncodingAlgorithm.LIBYAZ0)
        print(f"{GREEN}Encoded data length: {len(encoded_data)}{RESET}")
        print(encoded_data)

        decoded_data = RIISZS.decode(encoded_data)
        print(f"{GREEN}Decoded data: {decoded_data.decode()}{RESET}")  # Assuming it's a UTF-8 string for demonstration
    except RIISZSError as e:
        print(f"{RED}Error during encoding/decoding: {e}{RESET}")

    # 4. Encode with YAY0 and deinterlace
    try:
        yay0_encoded = RIISZS.encode_yay0(sample_data, EncodingAlgorithm.MK8)
        print(f"{GREEN}YAY0 Encoded data length: {len(yay0_encoded)}{RESET}")
        print(yay0_encoded)
    except RIISZSError as e:
        print(f"{RED}Error during YAY0 encoding/deinterlacing: {e}{RESET}")

    try: 
        print(f"{YELLOW}Trying to encode with an intentionally truncated dst buffer...{RESET}")
        dst = bytearray(3)
        encoded_data = RIISZS.encode_into(dst, sample_data, EncodingAlgorithm.LIBYAZ0)
        print("This shouldn't be reached!!!!")
    except RIISZSError as e:
        print(f"{GREEN}Expected error during YAZ0 encode: {e}{RESET}")

if __name__ == "__main__":
    main()
```
