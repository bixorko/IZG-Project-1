#include <student/gpu.h>

#include<iostream>
#include<stdio.h>
#include<assert.h>
#include<string.h>
#include<math.h>

/* CAST KODU BOLA PREBRATA Z drawPoints.cpp */

bool isPointInsideTriangle(Vec4 const*const p){
    // -Aw <= Ai <= +Aw
    for(int i=0;i<3;++i){
        if(p->data[i] <= -p->data[3])return false;
        if(p->data[i] >= +p->data[3])return false;
    }
    return true;
}

void PerspectiveDivisionTriangle(Vec4*const ndc,Vec4 const*const p){
    for(int a=0;a<3;++a)
        ndc->data[a] = p->data[a]/p->data[3];
    ndc->data[3] = p->data[3];
}

Vec4 computeFragPositionTriangle(Vec4 const&p,uint32_t width,uint32_t height){
    Vec4 res;
    res.data[0] = (p.data[0]*.5f+.5f)*width;
    res.data[1] = (p.data[1]*.5f+.5f)*height;
    res.data[2] = p.data[2];
    res.data[3] = p.data[3];
    return res;
}

void copyVertexAttributeTriangle(GPU const*const gpu,GPUAttribute*const att,GPUVertexPullerHead const*const head,uint64_t vertexId){
    if(!head->enabled)return;
    GPUBuffer const*const buf = gpu_getBuffer(gpu,head->bufferId);
    uint8_t const*ptr = (uint8_t*)buf->data;
    uint32_t const offset = (uint32_t)head->offset;
    uint32_t const stride = (uint32_t)head->stride;
    uint32_t const size   = (uint32_t)head->type;
    memcpy(att->data,ptr+offset + vertexId*stride,size);
}

void vertexPullerTriangle(GPUInVertex*const inVertex,GPUVertexPuller const*const vao,GPU const*const gpu,uint32_t vertexSHaderInvocation){
    uint32_t vertexId = vertexSHaderInvocation;

    if(gpu_isBuffer(gpu, vao->indices.bufferId)){

        GPUBuffer const *buff = gpu_getBuffer(gpu, vao->indices.bufferId);
        IndexType velkost = vao->indices.type;
        uint8_t *pom = (uint8_t *)(buff->data) + vertexId * velkost;

        if (velkost == 1){
            vertexId = *pom;
        }
        else if (velkost == 2){
            vertexId = *(uint16_t*) pom;
        }
        else if (velkost == 4){
            vertexId = *(uint32_t*) pom;
        }
        else{
            exit(1);
        }
    }

    inVertex->gl_VertexID = vertexId;

    for (int i = 0; i < MAX_ATTRIBUTES; i++) {
        copyVertexAttributeTriangle(gpu, inVertex->attributes + i, vao->heads + i, vertexId);
    }
}

void triangleRasterization(GPUInFragment*const inFragment,Vec4 ndc,GPU*const gpu,GPUOutVertex const*const outVertex){
    Vec4 coord = computeFragPositionTriangle(ndc,gpu->framebuffer.width,gpu->framebuffer.height);
    inFragment->gl_FragCoord = coord;
    memcpy(inFragment->attributes[0].data,outVertex->attributes[0].data,sizeof(Vec4));
}

void perFragmentOperation(GPUOutFragment const*const outFragment,GPU*const gpu,Vec4 ndc){
    Vec4 coord = computeFragPositionTriangle(ndc,gpu->framebuffer.width,gpu->framebuffer.height);
    GPUFramebuffer*const frame = &gpu->framebuffer;
    if(coord.data[0] < 0 || coord.data[0] >= frame->width)return;
    if(coord.data[1] < 0 || coord.data[1] >= frame->height)return;
    if(isnan(coord.data[0]))return;
    if(isnan(coord.data[1]))return;
    uint32_t const pixCoord = frame->width*(int)coord.data[1]+(int)coord.data[0];

    frame->color[pixCoord] = outFragment->gl_FragColor;
}

/**
 * @brief This function draws points
 *
 * @param gpu gpu
 * @param nofVertices number of vertices
 */
void gpu_drawTriangles(GPU*const gpu,uint32_t nofVertices){
    GPUProgram      const* prg = gpu_getActiveProgram(gpu);
    GPUVertexPuller const* vao = gpu_getActivePuller (gpu);

    GPUVertexShaderData   vd;
    GPUFragmentShaderData fd;

    vd.uniforms = &prg->uniforms;

    for(uint32_t v=0;v<nofVertices;++v){

        vertexPullerTriangle(&vd.inVertex,vao,gpu,v);

        prg->vertexShader(&vd);

        Vec4 pos;
        copy_Vec4(&pos,&vd.outVertex.gl_Position);
        if(!isPointInsideTriangle(&pos))continue;

        Vec4 ndc;
        PerspectiveDivisionTriangle(&ndc,&pos);


        if ((v+1%3) == 0) {
            triangleRasterization(&fd.inFragment, ndc, gpu, &vd.outVertex);

            prg->fragmentShader(&fd);

            perFragmentOperation(&fd.outFragment, gpu, ndc);
        }
    }
}
