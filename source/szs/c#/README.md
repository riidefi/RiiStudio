# C# Bindings
C# bindings are provided for the following native library. It's important to link against the .dll version of the library on Windows, or the .so on MacOS/Linux.

## bindings
A library, provides the following API:
```cs
public static class szs
{
	public enum CompressionAlgorithm
	{
		WorstCaseEncoding,
		Nintendo,
		MkwSP,
		CTGP,
		Haroohie,
		CTlib,
		LibYaz0,
		MK8,
	}
	public static bool IsCompressed(byte[] data);
	public static byte[] Decode(byte[] compressedData);
	public static byte[] Encode(byte[] data, CompressionAlgorithm algorithm = CompressionAlgorithm.MK8);
	public static string GetVersion();
}
```

## simple_example
Example project using the API above:
```cs
using System.Text;

public class Program
{
  public static void Main()
  {
    string sampleText = "Hello, world! This is a sample string for testing the szs compression library.AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHello, world!";
    byte[] originalData = Encoding.UTF8.GetBytes(sampleText);

    Console.WriteLine("Original Data:");
    Console.WriteLine(sampleText);
    Console.WriteLine($"Original Size: {originalData.Length} bytes");
    Console.WriteLine();

    // Check if data is compressed
    bool isCompressed = szs.IsCompressed(originalData);
    Console.WriteLine($"Is Original Data Compressed? {isCompressed}");
    Console.WriteLine();

    // Encode data
    byte[] encodedData = szs.Encode(originalData, szs.CompressionAlgorithm.LibYaz0);
    Console.WriteLine($"Encoded Size: {encodedData.Length} bytes");
    Console.WriteLine();

    // Decode data
    byte[] decodedData = szs.Decode(encodedData);
    string decodedText = Encoding.UTF8.GetString(decodedData);
    Console.WriteLine("Decoded Data:");
    Console.WriteLine(decodedText);
    Console.WriteLine();

    // Check if encoded data is compressed
    bool isEncodedDataCompressed = szs.IsCompressed(encodedData);
    Console.WriteLine($"Is Encoded Data Compressed? {isEncodedDataCompressed}");
    Console.WriteLine();

    // Get library version
    string version = szs.GetVersion();
    Console.WriteLine($"SZS Library Version: {version}");
  }
}

```
