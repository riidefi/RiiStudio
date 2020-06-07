#pragma once

namespace riistudio::lib3d {

enum class PixelOcclusion {
  //! The texture does not use alpha.
  //!
  Opaque,

  //! The texture's alpha is binary: a pixel is either fully opaque or
  //! discarded.
  //!
  Stencil,

  //! Pixels may express a range of alpha values for nuanced, translucent
  //! expression.
  //!
  Translucent
};

} // namespace riistudio::lib3d
