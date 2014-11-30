/*
    Boost Software License - Version 1.0 - August 17th, 2003

    Permission is hereby granted, free of charge, to any person or organization
    obtaining a copy of the software and accompanying documentation covered by
    this license (the "Software") to use, reproduce, display, distribute,
    execute, and transmit the Software, and to prepare derivative works of the
    Software, and to permit third-parties to whom the Software is furnished to
    do so, all subject to the following:

    The copyright notices in the Software and this entire statement, including
    the above license grant, this restriction and the following disclaimer,
    must be included in all copies of the Software, in whole or in part, and
    all derivative works of the Software, unless such copies or derivative
    works are solely in the form of machine-executable object code generated by
    a source language processor.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
    SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
    FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#ifndef CTR_GL_C
#error This is a private CTRGL implementation file. Please use #include <gl.h> instead.
#endif

static void setUpAlphaTest()
{
    if (enableState & GL_ALPHA_TEST)
        GPUCMD_AddSingleParam(0x000F0104, 1 | (alphaTestState.func << 4) | (alphaTestState.ref << 8));
    else
        GPUCMD_AddSingleParam(0x000F0104, 0);
}

static void setUpBlending()
{
    GPUCMD_AddSingleParam(0x000F0103, blendState.blendColor);
    GPUCMD_AddSingleParam(0x000F0101,
            blendState.modeRGB | (blendState.modeAlpha << 8)
            | (blendState.srcRGB << 16) | (blendState.dstRGB << 20)
            | (blendState.srcAlpha << 24) | (blendState.dstAlpha << 28));
    // TODO: GL_BLEND enable/disable
    GPUCMD_AddSingleParam(0x00020100, 0x00000100);
}

static void setUpCulling()
{
    if (enableState & GL_CULL_FACE)
        GPUCMD_AddSingleParam(0x000F0040, 2 - (cullState.cullFace ^ cullState.frontFace));
    else
        GPUCMD_AddSingleParam(0x000F0040, 0);
}

static void setUpDepthTest()
{
    if (enableState & GL_DEPTH_TEST)
        GPUCMD_AddSingleParam(0x000F0107, 1 | (depthTestState.func << 4)
                | (depthTestState.colorMask << 8) | (depthTestState.mask << 12));
    else
        GPUCMD_AddSingleParam(0x000F0107, 0 | (depthTestState.colorMask << 8) | (depthTestState.mask << 12));
}

static void setUpShaders()
{
    DVLB_s* dvlb;
    DVLE_s* dvle;

    dvlb = shaderState.program->dvlb;
    dvle = &dvlb->DVLE[0];

    /* geometry shader related? */
    GPUCMD_AddSingleParam(0x00010229, 0x00000000);
    GPUCMD_AddSingleParam(0x00010244, 0x00000000);

    DVLP_SendCode(&dvlb->DVLP);
    DVLP_SendOpDesc(&dvlb->DVLP);
    DVLE_SendConstants(dvle);

    GPUCMD_AddSingleParam(0x00080229, 0x00000000);
    GPUCMD_AddSingleParam(0x000F02BA, 0x7FFF0000|(dvle->mainOffset & 0xFFFF)); //set entrypoint

    GPUCMD_AddSingleParam(0x000F0252, 0x00000000); // should all be part of DVLE_SendOutmap ?

    DVLE_SendOutmap(dvle);

    //?
    GPUCMD_AddSingleParam(0x000F0064, 0x00000001);
    GPUCMD_AddSingleParam(0x000F006F, 0x00000703);
}

static void setUpStencil()
{
    static const uint8_t replace = 0x00;        /* TODO: how should this be set */

    if (enableState & GL_STENCIL_TEST)
        GPUCMD_AddSingleParam(0x000F0105, 1
                | (stencilState.func << 4)
                | (replace << 8)
                | (stencilState.ref << 16)
                | (stencilState.mask << 24));
    else
        GPUCMD_AddSingleParam(0x000F0105, 0);

    GPUCMD_AddSingleParam(0x000F0106, stencilState.sfail | (stencilState.dpfail << 4) | (stencilState.dppass << 8));
}

static void setUpTexturing()
{
    int i;

    /* enables texcoord outputs */
    GPUCMD_AddSingleParam(0x0002006F, texturingState.enableTextures << 8);

    /* enables texture units */
    GPUCMD_AddSingleParam(0x000F0080, 0x00011000 | texturingState.enableTextures);

    /* TexEnv */
    for (i = 0; i < NUM_TEXENV; i++)
    {
        static const u8 GPU_TEVID[] = {0xC0, 0xC8, 0xD0, 0xD8, 0xF0, 0xF8};

        GLtexEnvStateCTR* env;
        u32 param[0x5];

        uint16_t alphaSources, rgbSources;
        uint16_t alphaOperands, rgbOperands;

        if (!(dirtyTexEnv & (1 << i)))
            continue;

        memset(param, 0x00, 5 * 4);
        env = &texturingState.env[i];

        /* TODO: probably cache these */
        alphaSources =  (env->src0Alpha | (env->src1Alpha << 4) | (env->src2Alpha << 8));
        rgbSources =    (env->src0RGB | (env->src1RGB << 4) | (env->src2RGB << 8));
        alphaOperands = (env->operand0Alpha | (env->operand1Alpha << 4) | (env->operand2Alpha << 8));
        rgbOperands =   (env->operand0RGB | (env->operand1RGB << 4) | (env->operand2RGB << 8));

        param[0x0] = (alphaSources << 16) | rgbSources;
        param[0x1] = (alphaOperands << 12) | rgbOperands;
        param[0x2] = (env->combineAlpha << 16) | env->combineRGB;
        param[0x3] = env->constant;
        param[0x4] = 0x00000000;        /* unused */

        GPUCMD_Add(0x800F0000 | GPU_TEVID[i], param, 0x00000005);
    }

    for (i = 0; i < NUM_TEXUNITS; i++)
    {
        GLtextureCTR* tex;

        if (!(dirtyTexUnits & (1 << i)))
            continue;

        if (!(texturingState.enableTextures & (1 << i)))
            continue;

        tex = texturingState.texUnits[i].boundTexture;

        /* TODO: magFilter, minFilter, format */
        /* TODO: cache tex->data phys addr */

        static const int magFilter = GPU_TEXTURE_MAG_FILTER(GPU_NEAREST);
        static const int minFilter = GPU_TEXTURE_MIN_FILTER(GPU_NEAREST);

        gpuSetTexture(i,
                (void*) osConvertVirtToPhys((u32)(tex->data)),
                tex->w, tex->h, magFilter | minFilter, 0);
    }
}

static void setUpVertexArrays()
{
    int numAttributes;
    int i;

    uint64_t attributeFormats;
    uint16_t attributeMask;
    uint64_t attributePermutation;

    uint32_t bufferOffsets[1];
    uint64_t bufferPermutations[1];
    uint8_t bufferNumAttributes[1];

    numAttributes = vertexArraysState.numAttribs;
    attributeFormats = 0;
    attributeMask = 0xFFC;  /* what is this? */
    attributePermutation = 0;

    for (i = 0; i < numAttributes; i++)
    {
        GLvertexAttribCTR* at;

        at = &vertexArraysState.attribs[i];
        attributeFormats |= ((at->size << 2) | at->type) << (i * 4);
        attributePermutation |= i << (i * 4);
    }

    bufferOffsets[0] = 0x00000000;
    bufferPermutations[0] = attributePermutation;
    bufferNumAttributes[0] = numAttributes;

    GPU_SetAttributeBuffers(numAttributes,
        (u32*) osConvertVirtToPhys((u32) __linear_heap),
        attributeFormats, attributeMask, attributePermutation,
        1, bufferOffsets, bufferPermutations, bufferNumAttributes);
}
