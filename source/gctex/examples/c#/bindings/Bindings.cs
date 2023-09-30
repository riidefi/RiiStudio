//
// gctex C# bindings
// - riidefi, 2023
//
// Crate page: https://crates.io/crates/gctex
// Github: https://github.com/riidefi/RiiStudio/blob/master/source/gctex/
//

using System.Runtime.InteropServices;
using System.Text;

public static class gctex_native
{
  private const string DllName = "gctex_v13.dll";

  //
  // For the following APIs, keep in sync with `gctex.h` (https://github.com/riidefi/RiiStudio/blob/master/source/gctex/include/gctex.h)
  //

  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
  public static extern uint rii_compute_image_size(uint format, uint width, uint height);

  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
  public static extern uint rii_compute_image_size_mip(uint format, uint width, uint height, uint number_of_images);

  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
  public static extern void rii_encode(uint format, IntPtr dst, uint dst_len, IntPtr src, uint src_len, uint width, uint height);

  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
  public static extern void rii_decode(IntPtr dst, uint dst_len, IntPtr src, uint src_len, uint width, uint height, uint texformat, IntPtr tlut, uint tlut_len, uint tlutformat);

  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
  public static extern int gctex_get_version_unstable_api(byte[] buf, uint len);
}

public static class gctex
{
  public static uint ComputeImageSize(uint format, uint width, uint height)
  {
    return gctex_native.rii_compute_image_size(format, width, height);
  }

  public static uint ComputeImageSizeMip(uint format, uint width, uint height, uint number_of_images)
  {
    return gctex_native.rii_compute_image_size_mip(format, width, height, number_of_images);
  }

  public static void Encode(uint format, byte[] destination, byte[] source, uint width, uint height)
  {
    if (destination.Length < gctex_native.rii_compute_image_size(format, width, height))
      throw new ArgumentException("Destination buffer is too small.");

    GCHandle dstHandle = GCHandle.Alloc(destination, GCHandleType.Pinned);
    GCHandle srcHandle = GCHandle.Alloc(source, GCHandleType.Pinned);

    try
    {
      gctex_native.rii_encode(format, dstHandle.AddrOfPinnedObject(), (uint)destination.Length, srcHandle.AddrOfPinnedObject(), (uint)source.Length, width, height);
    }
    finally
    {
      dstHandle.Free();
      srcHandle.Free();
    }
  }

  public static byte[] Decode(byte[] source, uint width, uint height, uint texformat, byte[] tlut, uint tlutformat)
  {
    byte[] destination = new byte[4 * width * height];  // Assuming 32-bit raw color output

    GCHandle dstHandle = GCHandle.Alloc(destination, GCHandleType.Pinned);
    GCHandle srcHandle = GCHandle.Alloc(source, GCHandleType.Pinned);
    GCHandle tlutHandle = GCHandle.Alloc(tlut, GCHandleType.Pinned);

    try
    {
      gctex_native.rii_decode(dstHandle.AddrOfPinnedObject(), (uint)destination.Length, srcHandle.AddrOfPinnedObject(), (uint)source.Length, width, height, texformat, tlutHandle.AddrOfPinnedObject(), (uint)tlut.Length, tlutformat);
    }
    finally
    {
      dstHandle.Free();
      srcHandle.Free();
      tlutHandle.Free();
    }

    return destination;
  }

  public static string GetVersion()
  {
    byte[] buffer = new byte[256];
    int length = gctex_native.gctex_get_version_unstable_api(buffer, (uint)buffer.Length);
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
