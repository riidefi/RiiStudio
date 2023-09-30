# C# Bindings

C# bindings are provided for the following native library. It's important to link against the .dll version of the library on Windows, or the .so on MacOS/Linux.

## bindings
A library, provides the following API:
```cs
public static class rsmeshopt
{
  public enum StripifyAlgo
  {
    NvTriStripPort = 0,
    TriStripper,
    MeshOpt,
    Draco,
    DracoDegen,
    Haroohie,
  }
  public static List<uint> Stripify(
      StripifyAlgo algo, 
      List<uint> indexData,
      List<Vec3> vertexData, 
      uint restart = 0xFFFFFFFF);
  public static List<uint> MakeFans(
      List<uint> indexData, 
      uint restart,
      uint minLen, 
      uint maxRuns);
}
```

## simple_example
Example project using the API above:
```cs
namespace RsMeshOptExample
{
  class Program
  {
    static void Main()
    {
      try
      {
        string version = rsmeshopt.GetVersion();
        Console.WriteLine($"rsmeshopt.dll Version: {version}");
      }
      catch (InvalidOperationException ex)
      {
        Console.WriteLine($"Error retrieving version: {ex.Message}");
      }
    }
  }
}
```
