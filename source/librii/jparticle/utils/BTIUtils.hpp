namespace librii::jpa {

Result<void> importBTI(std::vector<librii::jpa::TextureBlock>& tex);
Result<void> replaceBTI(const char* btiName, librii::jpa::TextureBlock& data);
void exportBTI(const char* btiName, librii::jpa::TextureBlock data);

} // namespace librii::jpa
