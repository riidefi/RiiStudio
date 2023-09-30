//
// rsmneshopt C# bindings
// - riidefi, 2023
//
// Crate page: N/A
// Github: https://github.com/riidefi/RiiStudio/blob/master/source/rsmeshopt/
//

using System.Collections;
using System.Runtime.InteropServices;
using System.Text;

public static class rsmeshopt_native
{
  private const string DllName = "rsmeshopt.dll";

  //
  // For the following APIs, keep in sync with `rsmeshopt.h` (https://github.com/riidefi/RiiStudio/blob/master/source/rsmeshopt/include/rsmeshopt.h)
  //

  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
  public static extern uint rii_stripify(
      IntPtr dst,
      uint algo,
      uint[] indices,
      uint num_indices,
      float[] positions,
      uint num_positions,
      uint restart
  );

  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
  public static extern uint rii_makefans(
      IntPtr dst,
      uint[] indices,
      uint num_indices,
      uint restart,
      uint min_len,
      uint max_runs
  );

  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
  public static extern int rsmeshopt_get_version_unstable_api(byte[] buf, uint len);
}

public static class rsmeshopt
{
  public struct Vec3
  {
    public float X, Y, Z;
  }

  public enum StripifyAlgo
  {
    NvTriStripPort = 0,
    TriStripper,
    MeshOpt,
    Draco,
    DracoDegen,
    Haroohie,
  }

  private static uint StripifyBound(uint x)
  {
    return (x / 3) * 5;
  }

  // Note: Currently, vertexData is *only* needed for the StripifyAlgo::Draco algorithm
  public unsafe static List<uint> Stripify(
      StripifyAlgo algo,
      List<uint> indexData,
      List<Vec3> vertexData,
      uint restart = 0xFFFFFFFF)
  {
    uint bound = StripifyBound((uint)indexData.Count);
    uint[] indices = new uint[bound];

    fixed (uint* ind = &indices[0])
    {
      uint resultSize = rsmeshopt_native.rii_stripify(
          (IntPtr)ind,
          (uint)algo,
          indexData.ToArray(),
          (uint)indexData.Count,
          vertexData.ConvertAll(v => new float[] { v.X, v.Y, v.Z }).SelectMany(f => f).ToArray(),
          (uint)vertexData.Count * 3,
          restart
       );

      if (resultSize == 0)
      {
        throw new Exception("[rsmeshopt] rii_stripify failed.");
      }

      int[] result = new int[resultSize];
      Marshal.Copy((IntPtr)ind, result, 0, result.Length);

      return result.Select(x => (uint)x).ToList();
    }
  }

  public unsafe static List<uint> MakeFans(
      List<uint> indexData,
      uint restart,
      uint minLen,
      uint maxRuns)
  {
    uint bound = StripifyBound((uint)indexData.Count);
    uint[] indices = new uint[bound];

    fixed (uint* ind = &indices[0])
    {
      uint resultSize = rsmeshopt_native.rii_makefans(
      (IntPtr)ind,
      indexData.ToArray(),
      (uint)indexData.Count,
      restart,
      minLen,
      maxRuns);

      int[] result = new int[resultSize];
      Marshal.Copy((IntPtr)ind, result, 0, result.Length);

      return result.Select(x => (uint)x).ToList();
    }

  }
  public static string GetVersion()
  {
    byte[] buffer = new byte[256];
    int length = rsmeshopt_native.rsmeshopt_get_version_unstable_api(buffer, (uint)buffer.Length);
    if (length > 0 && length < 256)
    {
      return Encoding.UTF8.GetString(buffer, 0, length);
    }
    else
    {
      throw new InvalidOperationException("Failed to retrieve version.");
    }
  }
}
