/****************************************************************************
 Copyright (c) 2015-2016 Chukong Technologies Inc.
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

 ****************************************************************************/
#include "2d/CCCameraBackgroundBrush.h"
#include <stddef.h> // offsetof
#include "base/ccTypes.h"
#include "2d/CCCamera.h"
#include "base/ccMacros.h"
#include "base/ccUtils.h"
#include "base/CCConfiguration.h"
#include "base/CCDirector.h"
#include "renderer/CCRenderer.h"
#include "renderer/CCRenderState.h"
#include "renderer/CCTextureCube.h"
#include "renderer/ccShaders.h"

#if CC_ENABLE_CACHE_TEXTURE_DATA
#include "base/CCEventCustom.h"
#include "base/CCEventListenerCustom.h"
#include "base/CCEventType.h"
#include "base/CCEventDispatcher.h"
#endif

NS_CC_BEGIN

CameraBackgroundBrush::CameraBackgroundBrush()
{
    
}

CameraBackgroundBrush::~CameraBackgroundBrush()
{
    CC_SAFE_RELEASE_NULL(_programState);
}

CameraBackgroundBrush* CameraBackgroundBrush::createNoneBrush()
{
    auto ret = new (std::nothrow) CameraBackgroundBrush();
    ret->init();
    
    ret->autorelease();
    return ret;
}

CameraBackgroundColorBrush* CameraBackgroundBrush::createColorBrush(const Color4F& color, float depth)
{
    return CameraBackgroundColorBrush::create(color, depth);
}

CameraBackgroundDepthBrush* CameraBackgroundBrush::createDepthBrush(float depth)
{
    return CameraBackgroundDepthBrush::create(depth);
}

//////////////////////////////////////////////////////////////////////////////////////////

CameraBackgroundDepthBrush::CameraBackgroundDepthBrush()
: _depth(0.f)
, _clearColor(false)
#if CC_ENABLE_CACHE_TEXTURE_DATA
    , _backToForegroundListener(nullptr)
#endif
{
#if CC_ENABLE_CACHE_TEXTURE_DATA
    _backToForegroundListener = EventListenerCustom::create(EVENT_RENDERER_RECREATED, [this](EventCustom*)
        {
            initBuffer();
        }
    );
    Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(_backToForegroundListener, -1);
#endif
}
CameraBackgroundDepthBrush::~CameraBackgroundDepthBrush()
{
#if CC_ENABLE_CACHE_TEXTURE_DATA
    Director::getInstance()->getEventDispatcher()->removeEventListener(_backToForegroundListener);
#endif
}

CameraBackgroundDepthBrush* CameraBackgroundDepthBrush::create(float depth)
{
    auto ret = new (std::nothrow) CameraBackgroundDepthBrush();
    
    if (nullptr != ret && ret->init())
    {
        ret->_depth = depth;
        ret->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(ret);
    }

    return ret;
}

bool CameraBackgroundDepthBrush::init()
{
    CC_SAFE_RELEASE_NULL(_programState);
    auto* program = backend::Program::getBuiltinProgram(backend::ProgramType::CAMERA_CLEAR);
    _programState = new backend::ProgramState(program);

    _locDepth = _programState->getUniformLocation("dpeth");

    auto &pipelineDescriptor = _customCommand.getPipelineDescriptor();
    pipelineDescriptor.programState = _programState;

    auto layout = _programState->getVertexLayout();
    const auto& attributeInfo = _programState->getProgram()->getActiveAttributes();
    auto iter = attributeInfo.find("a_position");
    if(iter != attributeInfo.end())
    {
        layout->setAttribute("a_position", iter->second.location, backend::VertexFormat::FLOAT3, offsetof(V3F_C4B_T2F, vertices), false);
    }
    iter = attributeInfo.find("a_color");
    if(iter != attributeInfo.end())
    {
        layout->setAttribute("a_color", iter->second.location, backend::VertexFormat::UBYTE4, offsetof(V3F_C4B_T2F, colors), true);
    }
    iter = attributeInfo.find("a_texCoord");
    if(iter != attributeInfo.end())
    {
        layout->setAttribute("a_texCoord", iter->second.location, backend::VertexFormat::FLOAT2, offsetof(V3F_C4B_T2F, texCoords), true);
    }
    layout->setLayout(sizeof(_vertices[0]));

    _vertices.resize(4);
    _vertices[0].vertices = Vec3(-1, -1, 0);
    _vertices[1].vertices = Vec3(1, -1, 0);
    _vertices[2].vertices = Vec3(1, 1, 0);
    _vertices[3].vertices = Vec3(-1, 1, 0);

    _vertices[0].colors = _vertices[1].colors = _vertices[2].colors = _vertices[3].colors = Color4B(0, 0, 0, 1);

    _vertices[0].texCoords = Tex2F(0, 0);
    _vertices[1].texCoords = Tex2F(1, 0);
    _vertices[2].texCoords = Tex2F(1, 1);
    _vertices[3].texCoords = Tex2F(0, 1);

    _customCommand.setBeforeCallback(CC_CALLBACK_0(CameraBackgroundDepthBrush::onBeforeDraw, this));
    _customCommand.setAfterCallback(CC_CALLBACK_0(CameraBackgroundDepthBrush::onAfterDraw, this));

    initBuffer();
    return true;
}

void CameraBackgroundDepthBrush::initBuffer()
{
    uint16_t indices[6] = { 0, 1, 2, 2, 3, 0 };
    _customCommand.createVertexBuffer(sizeof(_vertices[0]), _vertices.size(), CustomCommand::BufferUsage::STATIC);
    _customCommand.createIndexBuffer(CustomCommand::IndexFormat::U_SHORT, sizeof(indices) / sizeof(indices[0]), CustomCommand::BufferUsage::STATIC);

    _customCommand.updateVertexBuffer(_vertices.data(), sizeof(_vertices[0]) * _vertices.size());
    _customCommand.updateIndexBuffer(indices, sizeof(indices));
}

void CameraBackgroundDepthBrush::drawBackground(Camera* /*camera*/)
{
    // `clear screen` should be executed before other commands
    _groupCommand.init(-1.0f);
    _customCommand.init(0.0f);

    auto *renderer = Director::getInstance()->getRenderer();

    renderer->addCommand(&_groupCommand);
    renderer->pushGroup(_groupCommand.getRenderQueueID());

    auto &pipelineDescriptor    = _customCommand.getPipelineDescriptor();
    auto &blend                 = pipelineDescriptor.blendDescriptor;
    blend.writeMask             = _clearColor ? backend::ColorWriteMask::ALL : backend::ColorWriteMask::NONE;

    //draw
    _programState->setUniform(_locDepth, &_depth, sizeof(_depth));
    
    renderer->addCommand(&_customCommand);

    renderer->popGroup();
}

void CameraBackgroundDepthBrush::onBeforeDraw()
{
    auto *renderer = Director::getInstance()->getRenderer();
    _stateBlock.stencilWriteMask    = renderer->getStencilWriteMask();
    _stateBlock.depthTest           = renderer->getDepthTest();
    _stateBlock.compareFunc         = renderer->getDepthCompareFunction();

    renderer->setStencilWriteMask(0);
    renderer->setDepthTest(true);
    renderer->setDepthCompareFunction(backend::CompareFunction::ALWAYS);
}

void CameraBackgroundDepthBrush::onAfterDraw()
{
    auto *renderer = Director::getInstance()->getRenderer();
    renderer->setStencilWriteMask(_stateBlock.stencilWriteMask);
    renderer->setDepthTest(_stateBlock.depthTest);
    renderer->setDepthCompareFunction(_stateBlock.compareFunc);
}

//////////////////////////////////////////////////////////////////////////////////////////

CameraBackgroundColorBrush::CameraBackgroundColorBrush()
: _color(0.f, 0.f, 0.f, 0.f)
{
    
}

CameraBackgroundColorBrush::~CameraBackgroundColorBrush()
{
    
}

bool CameraBackgroundColorBrush::init()
{
    CameraBackgroundDepthBrush::init();
    this->_clearColor = true;
    return true;
}

void CameraBackgroundColorBrush::drawBackground(Camera* camera)
{
    BlendFunc op = { BlendFunc::ALPHA_NON_PREMULTIPLIED.src, BlendFunc::ALPHA_NON_PREMULTIPLIED.dst };

    auto &blend = _customCommand.getPipelineDescriptor().blendDescriptor;
    blend.sourceRGBBlendFactor      = blend.sourceAlphaBlendFactor = op.src;
    blend.destinationRGBBlendFactor = blend.destinationAlphaBlendFactor = op.dst;
    blend.blendEnabled              = true;

    CameraBackgroundDepthBrush::drawBackground(camera);
}

void CameraBackgroundColorBrush::setColor(const Color4F& color)
{
    for (auto &vert : _vertices)
    {
        vert.colors = Color4B(color);
    }
    _customCommand.updateVertexBuffer(_vertices.data(), sizeof(_vertices[0]) * _vertices.size());
}

CameraBackgroundColorBrush* CameraBackgroundColorBrush::create(const Color4F& color, float depth)
{
    auto ret = new (std::nothrow) CameraBackgroundColorBrush();

    if (nullptr != ret && ret->init())
    {
        ret->setColor(color);
        ret->setDepth(depth);
        ret->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(ret);
    }

    return ret;
}
NS_CC_END
