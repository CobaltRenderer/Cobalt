// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Geometry buffer methods
//----------------------------------------------------------------------------------------
IVertexBuffer::unique_ptr IRenderer::CreateVertexBuffer()
{
	return IVertexBuffer::unique_ptr(CreateVertexBufferInternal());
}

//----------------------------------------------------------------------------------------
IIndexBuffer::unique_ptr IRenderer::CreateIndexBuffer()
{
	return IIndexBuffer::unique_ptr(CreateIndexBufferInternal());
}

//----------------------------------------------------------------------------------------
// Image buffer methods
//----------------------------------------------------------------------------------------
ITextureBuffer1D::unique_ptr IRenderer::CreateTextureBuffer1D()
{
	return ITextureBuffer1D::unique_ptr(CreateTextureBuffer1DInternal());
}

//----------------------------------------------------------------------------------------
ITextureBuffer2D::unique_ptr IRenderer::CreateTextureBuffer2D()
{
	return ITextureBuffer2D::unique_ptr(CreateTextureBuffer2DInternal());
}

//----------------------------------------------------------------------------------------
ITextureBuffer3D::unique_ptr IRenderer::CreateTextureBuffer3D()
{
	return ITextureBuffer3D::unique_ptr(CreateTextureBuffer3DInternal());
}

//----------------------------------------------------------------------------------------
ITextureBufferCube::unique_ptr IRenderer::CreateTextureBufferCube()
{
	return ITextureBufferCube::unique_ptr(CreateTextureBufferCubeInternal());
}

//----------------------------------------------------------------------------------------
ITextureBuffer1DArray::unique_ptr IRenderer::CreateTextureBuffer1DArray()
{
	return ITextureBuffer1DArray::unique_ptr(CreateTextureBuffer1DArrayInternal());
}

//----------------------------------------------------------------------------------------
ITextureBuffer2DArray::unique_ptr IRenderer::CreateTextureBuffer2DArray()
{
	return ITextureBuffer2DArray::unique_ptr(CreateTextureBuffer2DArrayInternal());
}

//----------------------------------------------------------------------------------------
ITextureBufferCubeArray::unique_ptr IRenderer::CreateTextureBufferCubeArray()
{
	return ITextureBufferCubeArray::unique_ptr(CreateTextureBufferCubeArrayInternal());
}

//----------------------------------------------------------------------------------------
// Image sampler methods
//----------------------------------------------------------------------------------------
ITextureSampler1D::unique_ptr IRenderer::CreateTextureSampler1D()
{
	return ITextureSampler1D::unique_ptr(CreateTextureSampler1DInternal());
}

//----------------------------------------------------------------------------------------
ITextureSampler2D::unique_ptr IRenderer::CreateTextureSampler2D()
{
	return ITextureSampler2D::unique_ptr(CreateTextureSampler2DInternal());
}

//----------------------------------------------------------------------------------------
ITextureSampler3D::unique_ptr IRenderer::CreateTextureSampler3D()
{
	return ITextureSampler3D::unique_ptr(CreateTextureSampler3DInternal());
}

//----------------------------------------------------------------------------------------
ITextureSamplerCube::unique_ptr IRenderer::CreateTextureSamplerCube()
{
	return ITextureSamplerCube::unique_ptr(CreateTextureSamplerCubeInternal());
}

//----------------------------------------------------------------------------------------
ITextureSampler1DArray::unique_ptr IRenderer::CreateTextureSampler1DArray()
{
	return ITextureSampler1DArray::unique_ptr(CreateTextureSampler1DArrayInternal());
}

//----------------------------------------------------------------------------------------
ITextureSampler2DArray::unique_ptr IRenderer::CreateTextureSampler2DArray()
{
	return ITextureSampler2DArray::unique_ptr(CreateTextureSampler2DArrayInternal());
}

//----------------------------------------------------------------------------------------
ITextureSamplerCubeArray::unique_ptr IRenderer::CreateTextureSamplerCubeArray()
{
	return ITextureSamplerCubeArray::unique_ptr(CreateTextureSamplerCubeArrayInternal());
}

//----------------------------------------------------------------------------------------
// Data array methods
//----------------------------------------------------------------------------------------
IDataArray::unique_ptr IRenderer::CreateDataArray()
{
	return IDataArray::unique_ptr(CreateDataArrayInternal());
}

//----------------------------------------------------------------------------------------
IDataArrayOutput::unique_ptr IRenderer::CreateDataArrayOutput()
{
	return IDataArrayOutput::unique_ptr(CreateDataArrayOutputInternal());
}

//----------------------------------------------------------------------------------------
ITexelArray::unique_ptr IRenderer::CreateTexelArray()
{
	return ITexelArray::unique_ptr(CreateTexelArrayInternal());
}

//----------------------------------------------------------------------------------------
ITexelArrayOutput::unique_ptr IRenderer::CreateTexelArrayOutput()
{
	return ITexelArrayOutput::unique_ptr(CreateTexelArrayOutputInternal());
}

//----------------------------------------------------------------------------------------
// Batch methods
//----------------------------------------------------------------------------------------
ITransferBatch::unique_ptr IRenderer::CreateTransferBatch(ITransferBatch::StartTiming startTiming, ITransferBatch::EndTiming endTiming)
{
	return ITransferBatch::unique_ptr(CreateTransferBatchInternal(startTiming, endTiming));
}

//----------------------------------------------------------------------------------------
// Frame buffer methods
//----------------------------------------------------------------------------------------
IFrameBuffer::unique_ptr IRenderer::CreateFrameBuffer()
{
	return IFrameBuffer::unique_ptr(CreateFrameBufferInternal());
}

//----------------------------------------------------------------------------------------
IFrameBufferOutput::unique_ptr IRenderer::CreateFrameBufferOutput()
{
	return IFrameBufferOutput::unique_ptr(CreateFrameBufferOutputInternal());
}

//----------------------------------------------------------------------------------------
// State buffer methods
//----------------------------------------------------------------------------------------
IStateBuffer::unique_ptr IRenderer::CreateStateBuffer()
{
	return IStateBuffer::unique_ptr(CreateStateBufferInternal());
}

//----------------------------------------------------------------------------------------
IStateBufferLayout::unique_ptr IRenderer::CreateStateBufferLayout()
{
	return IStateBufferLayout::unique_ptr(CreateStateBufferLayoutInternal());
}

//----------------------------------------------------------------------------------------
// Render tree node methods
//----------------------------------------------------------------------------------------
IRenderPassNode::unique_ptr IRenderer::CreateRenderPassNode()
{
	return IRenderPassNode::unique_ptr(CreateRenderPassNodeInternal());
}

//----------------------------------------------------------------------------------------
IProgramNode::unique_ptr IRenderer::CreateProgramNode()
{
	return IProgramNode::unique_ptr(CreateProgramNodeInternal());
}

//----------------------------------------------------------------------------------------
IStateGroupNode::unique_ptr IRenderer::CreateStateGroupNode()
{
	return IStateGroupNode::unique_ptr(CreateStateGroupNodeInternal());
}

//----------------------------------------------------------------------------------------
IRenderableNode::unique_ptr IRenderer::CreateRenderableNode()
{
	return IRenderableNode::unique_ptr(CreateRenderableNodeInternal());
}

//----------------------------------------------------------------------------------------
IDefaultState::unique_ptr IRenderer::CreateDefaultState()
{
	return IDefaultState::unique_ptr(CreateDefaultStateInternal());
}

//----------------------------------------------------------------------------------------
// Program methods
//----------------------------------------------------------------------------------------
IShaderProgram::unique_ptr IRenderer::CreateShaderProgram()
{
	return IShaderProgram::unique_ptr(CreateShaderProgramInternal());
}

}} // namespace cobalt::graphics
