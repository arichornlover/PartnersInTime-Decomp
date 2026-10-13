/* Overlay5SpritePool_InitializeStatic (ov005, 0x02069DAC-0x02069DE8).
 * Binary-matched implementation in build/analysis/emit/Overlay5SpritePool_InitializeStatic.o
 */
#include <game/overlay005_pools.h>
#include <msl/object_register.h>
extern "C" {
extern Overlay5SpritePool data_ov005_0206a3d8;
extern MslDestructorNode data_ov005_0206a3cc;
Overlay5SpritePool *func_ov005_02068ec8(Overlay5SpritePool *);
}
#pragma define_section SceneInit ".init" abs32 RX
#pragma section SceneInit begin
void Overlay5SpritePool_InitializeStatic(void)
{
    data_ov005_0206a3d8.sprites = 0;
    data_ov005_0206a3d8.nodes = 0;
    data_ov005_0206a3d8.free_list = 0;
    data_ov005_0206a3d8.taken_list = 0;
    data_ov005_0206a3d8.count = 0;
    MSL_RegisterGlobalObject(&data_ov005_0206a3d8, (MslGlobalDestructor)func_ov005_02068ec8, &data_ov005_0206a3cc);
}
#pragma section SceneInit end