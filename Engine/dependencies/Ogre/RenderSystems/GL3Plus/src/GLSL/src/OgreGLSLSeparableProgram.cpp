/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org/

  Copyright (c) 2000-2014 Torus Knot Software Ltd

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
  -----------------------------------------------------------------------------
*/

#include "OgreGLSLSeparableProgram.h"
#include "OgreStringConverter.h"
#include "OgreGLSLShader.h"
#include "OgreGLSLProgramManager.h"
#include "OgreGpuProgramManager.h"
#include "OgreGLUtil.h"
#include "OgreLogManager.h"
#include "OgreGLUniformCache.h"
#include "OgreGL3PlusStateCacheManager.h"

namespace Ogre
{
    GLSLSeparableProgram::GLSLSeparableProgram(const GLShaderList& shaders) :
        GLSLProgram(shaders)
    {
    }

    GLSLSeparableProgram::~GLSLSeparableProgram()
    {
        OGRE_CHECK_GL_ERROR(glDeleteProgramPipelines(1, &mGLProgramPipelineHandle));
    }

    void GLSLSeparableProgram::compileAndLink()
    {
        // Ensure no monolithic programs are in use.
        OGRE_CHECK_GL_ERROR(glUseProgram(0));
        OGRE_CHECK_GL_ERROR(glGenProgramPipelines(1, &mGLProgramPipelineHandle));

        mLinked = true;

        for (auto s : mShaders)
        {
            if(!s) continue;

            if(!s->linkSeparable())
        {
                mLinked = false;
                return;
            }
        }

            GLenum ogre2gltype[GPT_COUNT] = {
                GL_VERTEX_SHADER_BIT,
                GL_FRAGMENT_SHADER_BIT,
                GL_GEOMETRY_SHADER_BIT,
                GL_TESS_EVALUATION_SHADER_BIT,
                GL_TESS_CONTROL_SHADER_BIT,
                GL_COMPUTE_SHADER_BIT
            };

            for (auto s : mShaders)
            {
            if(!s) continue;

                OGRE_CHECK_GL_ERROR(glUseProgramStages(mGLProgramPipelineHandle, ogre2gltype[s->getType()],
                                                       s->getGLProgramHandle()));
            }

            // Validate pipeline
            OGRE_CHECK_GL_ERROR(glValidateProgramPipeline(mGLProgramPipelineHandle));
            logObjectInfo( getCombinedName() + String("GLSL program pipeline validation result: "), mGLProgramPipelineHandle );

            //            if (getGLSupport()->checkExtension("GL_KHR_debug") || gl3wIsSupported(4, 3))
            //                glObjectLabel(GL_PROGRAM_PIPELINE, mGLProgramPipelineHandle, 0,
            //                                 (mVertexShader->getName() + "/" + mFragmentShader->getName()).c_str());
        }

    void GLSLSeparableProgram::activate(void)
    {
        if (!mLinked)
        {
            compileAndLink();

            extractLayoutQualifiers();
        }

        if (mLinked)
        {
            GLSLProgramManager::getSingleton().getStateCacheManager()->bindGLProgramPipeline(mGLProgramPipelineHandle);
        }
    }

    void GLSLSeparableProgram::updateUniforms(GpuProgramParametersSharedPtr params,
                                              uint16 mask, GpuProgramType fromProgType)
    {
        // determine if we need to transpose matrices when binding
        bool transpose = !mShaders[fromProgType] || mShaders[fromProgType]->getColumnMajorMatrices();

        OgreAssert(mShaders[fromProgType], "invalid program type");
        GLuint progID = mShaders[fromProgType]->getGLProgramHandle();
        GLUniformCache* uniformCache = mShaders[fromProgType]->getUniformCache();

        // Iterate through uniform reference list and update uniform values
        for (const auto it : params->getConstantDefinitions().map)
        {
            const GpuConstantDefinition* def = &it.second;
            if ((def->variability & mask) == 0) // masked
                continue;

            GLsizei glArraySize = (GLsizei)def->arraySize;

            bool shouldUpdate = true;

            switch (def->constType)
            {
            case GCT_INT1:
            case GCT_INT2:
            case GCT_INT3:
            case GCT_INT4:
            case GCT_SAMPLER1D:
            case GCT_SAMPLER1DSHADOW:
            case GCT_SAMPLER2D:
            case GCT_SAMPLER2DSHADOW:
            case GCT_SAMPLER2DARRAY:
            case GCT_SAMPLER3D:
            case GCT_SAMPLERCUBE:
                shouldUpdate =
                    uniformCache->updateUniform(def->logicalIndex, params->getIntPointer(def->physicalIndex),
                                                static_cast<GLsizei>(def->elementSize * def->arraySize * sizeof(int)));
                break;
            default:
                shouldUpdate = uniformCache->updateUniform(
                    def->logicalIndex, params->getFloatPointer(def->physicalIndex),
                    static_cast<GLsizei>(def->elementSize * def->arraySize * sizeof(float)));
                break;
            }
            if (!shouldUpdate)
            {
                continue;
            }

            // Get the index in the parameter real list
            switch (def->constType)
            {
            case GCT_FLOAT1:
                OGRE_CHECK_GL_ERROR(glProgramUniform1fv(progID, def->logicalIndex, glArraySize,
                                                        params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_FLOAT2:
                OGRE_CHECK_GL_ERROR(glProgramUniform2fv(progID, def->logicalIndex, glArraySize,
                                                        params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_FLOAT3:
                OGRE_CHECK_GL_ERROR(glProgramUniform3fv(progID, def->logicalIndex, glArraySize,
                                                        params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_FLOAT4:
                OGRE_CHECK_GL_ERROR(glProgramUniform4fv(progID, def->logicalIndex, glArraySize,
                                                        params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_2X2:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix2fv(progID, def->logicalIndex, glArraySize, transpose,
                                                              params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_3X3:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix3fv(progID, def->logicalIndex, glArraySize, transpose,
                                                              params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_4X4:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix4fv(progID, def->logicalIndex, glArraySize, transpose,
                                                              params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_SAMPLER1D:
            case GCT_SAMPLER1DSHADOW:
            case GCT_SAMPLER2D:
            case GCT_SAMPLER2DSHADOW:
            case GCT_SAMPLER2DARRAY:
            case GCT_SAMPLER3D:
            case GCT_SAMPLERCUBE:
            case GCT_SAMPLERRECT:
                // Samplers handled like 1-element ints
            case GCT_INT1:
                OGRE_CHECK_GL_ERROR(glProgramUniform1iv(progID, def->logicalIndex, glArraySize,
                                                        params->getIntPointer(def->physicalIndex)));
                break;
            case GCT_INT2:
                OGRE_CHECK_GL_ERROR(glProgramUniform2iv(progID, def->logicalIndex, glArraySize,
                                                        params->getIntPointer(def->physicalIndex)));
                break;
            case GCT_INT3:
                OGRE_CHECK_GL_ERROR(glProgramUniform3iv(progID, def->logicalIndex, glArraySize,
                                                        params->getIntPointer(def->physicalIndex)));
                break;
            case GCT_INT4:
                OGRE_CHECK_GL_ERROR(glProgramUniform4iv(progID, def->logicalIndex, glArraySize,
                                                        params->getIntPointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_2X3:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix2x3fv(progID, def->logicalIndex, glArraySize, GL_FALSE,
                                                                params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_2X4:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix2x4fv(progID, def->logicalIndex, glArraySize, GL_FALSE,
                                                                params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_3X2:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix3x2fv(progID, def->logicalIndex, glArraySize, GL_FALSE,
                                                                params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_3X4:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix3x4fv(progID, def->logicalIndex, glArraySize, GL_FALSE,
                                                                params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_4X2:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix4x2fv(progID, def->logicalIndex, glArraySize, GL_FALSE,
                                                                params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_4X3:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix4x3fv(progID, def->logicalIndex, glArraySize, GL_FALSE,
                                                                params->getFloatPointer(def->physicalIndex)));
                break;
            case GCT_DOUBLE1:
                OGRE_CHECK_GL_ERROR(glProgramUniform1dv(progID, def->logicalIndex, glArraySize,
                                                        params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_DOUBLE2:
                OGRE_CHECK_GL_ERROR(glProgramUniform2dv(progID, def->logicalIndex, glArraySize,
                                                        params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_DOUBLE3:
                OGRE_CHECK_GL_ERROR(glProgramUniform3dv(progID, def->logicalIndex, glArraySize,
                                                        params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_DOUBLE4:
                OGRE_CHECK_GL_ERROR(glProgramUniform4dv(progID, def->logicalIndex, glArraySize,
                                                        params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_DOUBLE_2X2:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix2dv(progID, def->logicalIndex, glArraySize, transpose,
                                                              params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_DOUBLE_3X3:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix3dv(progID, def->logicalIndex, glArraySize, transpose,
                                                              params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_DOUBLE_4X4:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix4dv(progID, def->logicalIndex, glArraySize, transpose,
                                                              params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_DOUBLE_2X3:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix2x3dv(progID, def->logicalIndex, glArraySize, transpose,
                                                                params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_DOUBLE_2X4:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix2x4dv(progID, def->logicalIndex, glArraySize, transpose,
                                                                params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_DOUBLE_3X2:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix3x2dv(progID, def->logicalIndex, glArraySize, transpose,
                                                                params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_DOUBLE_3X4:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix3x4dv(progID, def->logicalIndex, glArraySize, transpose,
                                                                params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_DOUBLE_4X2:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix4x2dv(progID, def->logicalIndex, glArraySize, transpose,
                                                                params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_MATRIX_DOUBLE_4X3:
                OGRE_CHECK_GL_ERROR(glProgramUniformMatrix4x3dv(progID, def->logicalIndex, glArraySize, transpose,
                                                                params->getDoublePointer(def->physicalIndex)));
                break;
            case GCT_UINT1:
            case GCT_BOOL1:
                OGRE_CHECK_GL_ERROR(glProgramUniform1uiv(progID, def->logicalIndex, glArraySize,
                                                         params->getUnsignedIntPointer(def->physicalIndex)));
                break;
            case GCT_UINT2:
            case GCT_BOOL2:
                OGRE_CHECK_GL_ERROR(glProgramUniform2uiv(progID, def->logicalIndex, glArraySize,
                                                         params->getUnsignedIntPointer(def->physicalIndex)));
                break;
            case GCT_UINT3:
            case GCT_BOOL3:
                OGRE_CHECK_GL_ERROR(glProgramUniform3uiv(progID, def->logicalIndex, glArraySize,
                                                         params->getUnsignedIntPointer(def->physicalIndex)));
                break;
            case GCT_UINT4:
            case GCT_BOOL4:
                OGRE_CHECK_GL_ERROR(glProgramUniform4uiv(progID, def->logicalIndex, glArraySize,
                                                         params->getUnsignedIntPointer(def->physicalIndex)));
                break;

            default:
                break;

            } // End switch
        }     // End for
    }

    void GLSLSeparableProgram::updateAtomicCounters(GpuProgramParametersSharedPtr params,
                                                    uint16 mask, GpuProgramType fromProgType)
    {
        // Iterate through the list of atomic counter buffers and update them as needed
        // GLAtomicBufferIterator currentBuffer = mGLAtomicBufferReferences.begin();
        // GLAtomicBufferIterator endBuffer = mGLAtomicBufferReferences.end();

        GLAtomicCounterReferenceIterator currentAtomicCounter = mGLAtomicCounterReferences.begin();
        GLAtomicCounterReferenceIterator endAtomicCounter = mGLAtomicCounterReferences.end();

        for (; currentAtomicCounter != endAtomicCounter; ++currentAtomicCounter)
        {
            if (fromProgType == currentAtomicCounter->mSourceProgType)
            {
                const GpuConstantDefinition* def = currentAtomicCounter->mConstantDef;
                if (def->variability & mask)
                {
                    GLsizei glArraySize = (GLsizei)def->arraySize;
                    GLuint glBinding = currentAtomicCounter->mBinding;
                    GLuint glOffset = currentAtomicCounter->mOffset;

                    // Get the buffer this atomic counter belongs to.
                    //TODO exception handling
                    HardwareCounterBufferSharedPtr atomic_buffer = mGLCounterBufferReferences[glBinding];

                    // Update the value.
                    atomic_buffer->writeData(glOffset, sizeof(GLuint) * glArraySize, params->getUnsignedIntPointer(def->physicalIndex));
                }
            }
        }
    }
}
