import ctypes
from enum import Enum

# Constants
DLL_PATH = ".\\szs.dll"

# Load the shared library
_lib_riiszs = ctypes.CDLL(DLL_PATH)

# Bindings Helper
def bind_function(name, argtypes, restype):
    func = getattr(_lib_riiszs, name)
    func.argtypes = argtypes
    func.restype = restype
    return func

# Function Bindings
_riiszs_is_compressed = bind_function("riiszs_is_compressed", [ctypes.c_void_p, ctypes.c_uint32], ctypes.c_uint32)
_riiszs_decoded_size = bind_function("riiszs_decoded_size", [ctypes.c_void_p, ctypes.c_uint32], ctypes.c_uint32)
_riiszs_decode = bind_function("riiszs_decode", [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32], ctypes.c_char_p)
_riiszs_decode_yay0_into = bind_function("riiszs_decode_yay0_into", [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32], ctypes.c_char_p)
_riiszs_encoded_upper_bound = bind_function("riiszs_encoded_upper_bound", [ctypes.c_uint32], ctypes.c_uint32)
_riiszs_encode_algo_fast = bind_function("riiszs_encode_algo_fast", [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32], ctypes.c_char_p)
_riiszs_free_error_message = bind_function("riiszs_free_error_message", [ctypes.c_char_p], None)
_szs_get_version_unstable_api = bind_function("szs_get_version_unstable_api", [ctypes.c_char_p, ctypes.c_uint32], ctypes.c_int32)
_riiszs_deinterlaced_upper_bound = bind_function("riiszs_deinterlaced_upper_bound", [ctypes.c_uint32], ctypes.c_uint32)
_riiszs_deinterlace_into = bind_function("riiszs_deinterlace_into", [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)], ctypes.c_char_p)

# Enums
class EncodingAlgorithm(Enum):
    WORST_CASE_ENCODING = 0
    NINTENDO = 1
    MKWSP = 2
    CTGP = 3
    HAROOHIE = 4
    CTLIB = 5
    LIBYAZ0 = 6
    MK8 = 7

class RIISZSError(Exception):
    pass

class RIISZS:
    @staticmethod
    def _handle_error(err):
        if err:
            decoded = err.decode()
            # This crashes for some reason, so we just leak it lol
            # _riiszs_free_error_message(err)
            raise RIISZSError(decoded)

    @staticmethod
    def _to_pointer(data: bytes) -> ctypes.POINTER(ctypes.c_byte):
        if isinstance(data, bytes):
            return (ctypes.c_byte * len(data)).from_buffer_copy(data)
        elif isinstance(data, bytearray):
            return (ctypes.c_byte * len(data)).from_buffer(data)
        else:
            raise TypeError("Expected bytes or bytearray, got {}".format(type(data).__name__))

    @staticmethod
    def get_version() -> str:
        s = ctypes.create_string_buffer(1024)
        len_ = _szs_get_version_unstable_api(s, 1024)
        if len_ <= 0 or len_ > 1024:
            raise RIISZSError("Unable to query version.")
        return s.value.decode()

    @staticmethod
    def is_compressed(src: bytes) -> bool:
        return bool(_riiszs_is_compressed(src, len(src)))

    @staticmethod
    def decoded_size(src: bytes) -> int:
        return _riiszs_decoded_size(RIISZS._to_pointer(src), len(src))

    @staticmethod
    def encoded_upper_bound(length: int) -> int:
        return _riiszs_encoded_upper_bound(length)

    @staticmethod
    def encode_into(dst: bytearray, src: bytes, algo: EncodingAlgorithm) -> int:
        used_len = ctypes.c_uint32()
        err = _riiszs_encode_algo_fast(RIISZS._to_pointer(dst), len(dst), RIISZS._to_pointer(src), len(src), ctypes.byref(used_len), algo.value)
        RIISZS._handle_error(err)
        return used_len.value

    @staticmethod
    def decode_into(dst: bytearray, src: bytes):
        err = _riiszs_decode(RIISZS._to_pointer(dst), len(dst), RIISZS._to_pointer(src), len(src))
        RIISZS._handle_error(err)

    @staticmethod
    def decode(src: bytes) -> bytearray:
        size = RIISZS.decoded_size(src)
        result = bytearray(size)
        RIISZS.decode_into(result, src)
        return result

    @staticmethod
    def encode(src: bytes, algo: EncodingAlgorithm) -> bytearray:
        size = RIISZS.encoded_upper_bound(len(src))
        result = bytearray(size)
        RIISZS.encode_into(result, src, algo)
        return result

    @staticmethod
    def decode_yay0_into(dst: bytearray, src: bytes):
        err = _riiszs_decode_yay0_into(RIISZS._to_pointer(dst), len(dst), RIISZS._to_pointer(src), len(src))
        RIISZS._handle_error(err)

    @staticmethod
    def decode_yay0(src: bytes) -> bytearray:
        size = RIISZS.decoded_size(src)
        result = bytearray(size)
        RIISZS.decode_yay0_into(result, src)
        return result

    @staticmethod
    def deinterlace_into(dst: bytearray, src: bytes) -> int:
        used_len = ctypes.c_uint32()
        err = _riiszs_deinterlace_into(RIISZS._to_pointer(dst), len(dst), RIISZS._to_pointer(src), len(src), ctypes.byref(used_len))
        RIISZS._handle_error(err)
        return used_len.value

    @staticmethod
    def deinterlace(src: bytes) -> bytearray:
        size = _riiszs_deinterlaced_upper_bound(len(src))
        result = bytearray(size)
        actual_size = RIISZS.deinterlace_into(result, src)
        return result[:actual_size]

    @staticmethod
    def encode_yay0(src: bytes, algo: EncodingAlgorithm) -> bytearray:
        encoded = RIISZS.encode(src, algo)
        return RIISZS.deinterlace(encoded)

def main():
    # ANSI escape codes for coloring
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    RESET = "\033[0m"
    
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
