/**
 * @file quick_functions.cpp
 *
 */
#include "quick_functions.hpp"
#include "gta_sa.hpp"

extern GtaSA gta;

static vec3f *operator_mul_sub_59C890(vec3f *a1, float *a2, float *a3);
static uint32_t RpSkinAtomicGetHAnimHierarchy_sub_7C7540(uint32_t a1);
static uint32_t skinAtomicGetHAnimHierarchCB_sub_734A20(uint32_t a1,
                                                        uint32_t a2);
static uint32_t RpClumpForAllAtomics_sub_749B70(uint32_t a1, uint32_t callback,
                                                uint32_t a3);
static uint32_t GetAnimHierarchyFromSkinClump_sub_734A40(uint32_t a1);
static int32_t RpHAnimIDGetIndex_sub_7C51A0(uint32_t a1, uint32_t bone_id);
static uint32_t RpHAnimHierarchyGetMatrixArray_sub_7C5120(uint32_t a1);

vec3f QuickFunctions::getBonePosition_sub_5E4280(uint32_t ped_addr,
                                                 uint32_t bone_id,
                                                 uint8_t is_dynamic)
{

    is_dynamic = false;
    // TODO: This function has unreversed block (0x532B20 function call). So,
    //       the whole idea to speed up the time of obtaining the coordinates of
    //       the player's bones by getting rid of callers will lose its meaning
    //       if the is_dynamic argument is true.

    uint32_t v5_ped_flags;                     // eax
    int AnimHierarchyFromSkinClump_sub_734A40; // eax
    int v8;                                    // eax
    int v14;                                   // esi
    vec3f *v15;                                // eax
    vec3f result;
    v5_ped_flags = gta.read<uint32_t>(ped_addr + 0x474);

    if (is_dynamic)
    {
        if ((v5_ped_flags & 0x400) == 0)
        {
            gta.call_thiscall_function(0x532B20, ped_addr, 0,
                                       0); // sub_532B20();
            gta.write<uint32_t>((uint32_t)ped_addr + 285,
                                gta.read<uint32_t>(ped_addr + 285) |
                                    0x400u); // ped_addr[285] |= 0x400u;
        }
    }
    else if ((v5_ped_flags & 0x400) == 0)
    {
        uint32_t tmp = gta.read<uint32_t>(ped_addr + 0x14);
        uint8_t matrix[0x40];
        vec3f point = gta.read<vec3f>(12 * bone_id + 0x8D13A8);
        gta.read_buf(tmp, sizeof(matrix), matrix);
        operator_mul_sub_59C890(&result, (float *)matrix, (float *)&point);
        return result;
    }
    uint32_t tmp = gta.read<uint32_t>(ped_addr + 0x18);
    AnimHierarchyFromSkinClump_sub_734A40 =
        GetAnimHierarchyFromSkinClump_sub_734A40(tmp);
    // AnimHierarchyFromSkinClump_sub_734A40 =
    // gta.call_cdecl_function(0x734A40, 1, tmp);
    // //GetAnimHierarchyFromSkinClump_sub_734A40(ped_addr[6]);

    if (AnimHierarchyFromSkinClump_sub_734A40)
    {
        v14 = RpHAnimIDGetIndex_sub_7C51A0(
            AnimHierarchyFromSkinClump_sub_734A40, bone_id);
        // v14 = gta.call_cdecl_function(0x7C51A0, 2,
        // AnimHierarchyFromSkinClump_sub_734A40, bone_id);
        v15 = (vec3f *)((v14 << 6) +
                        RpHAnimHierarchyGetMatrixArray_sub_7C5120(
                            AnimHierarchyFromSkinClump_sub_734A40) +
                        48);
        result = gta.read<vec3f>((uint32_t)v15);
    }
    else
    {
        v8 = gta.read<uint32_t>(ped_addr + 0x14);
        if (v8)
        {
            result = gta.read<vec3f>(v8 + 48);
        }
        else
        {
            result = gta.read<vec3f>(ped_addr + 4);
        }
    }
    return result;
}

static vec3f *operator_mul_sub_59C890(vec3f *a1, float *a2, float *a3)
{
    a1->x = a2[8] * a3[2] + a2[4] * a3[1] + *a2 * *a3 + a2[12];
    a1->y = a2[9] * a3[2] + a2[1] * *a3 + a2[5] * a3[1] + a2[13];
    a1->z = a2[10] * a3[2] + a2[2] * *a3 + a2[6] * a3[1] + a2[14];
    return a1;
}

static uint32_t RpSkinAtomicGetHAnimHierarchy_sub_7C7540(uint32_t a1)
{
    uint32_t result = gta.read<uint32_t>(0xC978A4);
    result = gta.read<uint32_t>(result + a1);
    return result;
}

static uint32_t skinAtomicGetHAnimHierarchCB_sub_734A20(uint32_t a1,
                                                        uint32_t a2)
{
    *(uint32_t *)a2 =
        RpSkinAtomicGetHAnimHierarchy_sub_7C7540(a1); // TODO: Check!
    return 0;
}

static uint32_t RpClumpForAllAtomics_sub_749B70(uint32_t a1, uint32_t callback,
                                                uint32_t a3)
{
    uint32_t v3; // eax
    uint32_t v4; // esi

    v3 = gta.read<uint32_t>(a1 + 8);
    // v3 = *(uint32_t**)(a1 + 8);
    if (v3 != (a1 + 8))
    {
        do
        {
            // v4 = (uint32_t*)*v3;
            v4 = gta.read<uint32_t>(v3);
            uint32_t (*cb)(uint32_t, uint32_t) =
                (uint32_t(*)(uint32_t, uint32_t))callback;
            if (!cb(v3 - 16 * 4, a3))
                break;
            v3 = v4;
        } while (v4 != (a1 + 8));
    }
    return a1;
    return 0;
}

static uint32_t GetAnimHierarchyFromSkinClump_sub_734A40(uint32_t a1)
{
    // return gta.call_cdecl_function(0x734A40, 1, a1);
    int v2 = 0; // [esp+0h] [ebp-4h] BYREF
    RpClumpForAllAtomics_sub_749B70(
        a1, (uint32_t)skinAtomicGetHAnimHierarchCB_sub_734A20, (uint32_t)&v2);
    return v2;
}

static int32_t RpHAnimIDGetIndex_sub_7C51A0(uint32_t a1, uint32_t bone_id)
{
    int result;  // eax
    uint32_t v3; // esi
    uint32_t v4; // edx
    uint32_t v5; // ecx

    result = -1;
    // v3 = *(uint32_t**)(a1 + 16);
    v3 = gta.read<uint32_t>(a1 + 16);
    // v4 = *(uint32_t*)(a1 + 4);
    v4 = gta.read<uint32_t>(a1 + 4);
    v5 = 0;
    if (v4 > 0)
    {
        while (bone_id != gta.read<uint32_t>(v3))
        {
            ++v5;
            v3 += 16;
            if (v5 >= v4)
                return result;
        }
        return v5;
    }
    return result;
}

static uint32_t RpHAnimHierarchyGetMatrixArray_sub_7C5120(uint32_t a1)
{
    return gta.read<uint32_t>(a1 + 8);
}
