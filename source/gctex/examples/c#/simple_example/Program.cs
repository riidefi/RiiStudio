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
