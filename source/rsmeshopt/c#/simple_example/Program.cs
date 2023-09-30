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
