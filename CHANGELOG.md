## [2.0.1](https://github.com/CobaltRenderer/Cobalt/compare/v2.0.0...v2.0.1) (2026-10-30)


### Bug Fixes

* Added workaround for Intel OpenGL driver defect with PBO transfers ([#29](https://github.com/CobaltRenderer/Cobalt/issues/29)) ([a6de624](https://github.com/CobaltRenderer/Cobalt/commit/a6de62475597234457889b19e8713b9362c4863c))
* Apply WSL crash workaround to EGL builds, and improve error logging. ([#10](https://github.com/CobaltRenderer/Cobalt/issues/10)) ([272c68f](https://github.com/CobaltRenderer/Cobalt/commit/272c68f02d95300c9ab6102bb486cef50d75e4ad))
* Corrected C API return type for Cobalt_TransferBatch_SubmitBatch ([#13](https://github.com/CobaltRenderer/Cobalt/issues/13)) ([c8e864b](https://github.com/CobaltRenderer/Cobalt/commit/c8e864b69ccb58ff04e4dee3205e1e618dace167))
* Fixed delayed release of GPU resources on OpenGL due to VAO lifetime ([#26](https://github.com/CobaltRenderer/Cobalt/issues/26)) ([10e5c91](https://github.com/CobaltRenderer/Cobalt/commit/10e5c91fabbf5b963eac321359b3bf24d366f4ce))
* Fixed issues with unbound vertex attributes ([#21](https://github.com/CobaltRenderer/Cobalt/issues/21)) ([cf2123f](https://github.com/CobaltRenderer/Cobalt/commit/cf2123ff4baa6981ba5555b9eb549db8ff49e110))
* Fixed memory leak when calling SetStateValue with a null StateId ([#18](https://github.com/CobaltRenderer/Cobalt/issues/18)) ([d86ead5](https://github.com/CobaltRenderer/Cobalt/commit/d86ead5cccfa3e777c7c15bebdc417346192d63f))
* Fixed OpenGL shader stage I/O naming issues during cross-compilation ([#27](https://github.com/CobaltRenderer/Cobalt/issues/27)) ([c29ad74](https://github.com/CobaltRenderer/Cobalt/commit/c29ad74eac827c7905d623db072c76b947b19d25))
* Fixed resource leak in D3D11 and D3D12 renderers ([#25](https://github.com/CobaltRenderer/Cobalt/issues/25)) ([de22767](https://github.com/CobaltRenderer/Cobalt/commit/de2276754556ed20634ff168ccdb78331d9462c4))
* Fixed various buffer alignment issues, especially with matrix types. ([#28](https://github.com/CobaltRenderer/Cobalt/issues/28)) ([d9e8fd2](https://github.com/CobaltRenderer/Cobalt/commit/d9e8fd20672ab2ef7d94ab57c65aa4894ffe5167))
* Fixed vsync lock under D3D11 when not requested ([#24](https://github.com/CobaltRenderer/Cobalt/issues/24)) ([ed39947](https://github.com/CobaltRenderer/Cobalt/commit/ed39947f05e3b0c97e8d779b66c727ade11e4e50))
* Improved handling and docs for out of bounds output captures ([#23](https://github.com/CobaltRenderer/Cobalt/issues/23)) ([6b3eb6f](https://github.com/CobaltRenderer/Cobalt/commit/6b3eb6f03778ad886be53bdbef07e502c0ef2e8f))
* Made IStateGroupNode::AddChildNode correctly abort on errors under D3D12 and Vulkan ([#15](https://github.com/CobaltRenderer/Cobalt/issues/15)) ([2951423](https://github.com/CobaltRenderer/Cobalt/commit/295142322fc175a1a6a9581b396d0a3ad705716f))
* OpenGL - Mark VAOs as generated so they don't leak ([#17](https://github.com/CobaltRenderer/Cobalt/issues/17)) ([f988ce0](https://github.com/CobaltRenderer/Cobalt/commit/f988ce0a5b655da6d7b0d35d388b03dfa8a59f5c))

## [2.0.0](https://github.com/CobaltRenderer/Cobalt/compare/186be0e63df8126aa867bc456f34bc7dcd102a64...v2.0.0) (2026-06-29)


### ⚠ BREAKING CHANGES

* Initial commit for v2.0 open-source release

### Features

* Initial commit for v2.0 open-source release ([186be0e](https://github.com/CobaltRenderer/Cobalt/commit/186be0e63df8126aa867bc456f34bc7dcd102a64))

