# C# Bindings

C# bindings are provided for the following native library. It's important to link against the .dll version of the library on Windows, or the .so on MacOS/Linux.

## bindings
A library, provides the following API:
```cs
public static class gctex
{
  public static uint ComputeImageSize(uint format, uint width, uint height);

  public static uint ComputeImageSizeMip(uint format, uint width, uint height, uint number_of_images);

  public static void Encode(uint format, byte[] destination, byte[] source, uint width, uint height);

  public static byte[] Decode(byte[] source, uint width, uint height, uint texformat, byte[] tlut, uint tlutformat);

  public static string GetVersion();
}
```

## simple_example
Example project using the API above:
```cs
namespace GcTexExample
{
  class Program
  {
    static void Main()
    {
      try
      {
        string version = gctex.GetVersion();
        Console.WriteLine($"gctex.dll Version: {version}");
      }
      catch (InvalidOperationException ex)
      {
        Console.WriteLine($"Error retrieving version: {ex.Message}");
      }
    }
  }
}
```
