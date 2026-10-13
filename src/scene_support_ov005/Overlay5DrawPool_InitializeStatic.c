/* Overlay5DrawPool_InitializeStatic (ov005, 0x02069DE8-0x02069E24).
 * Binary-matched implementation in build/analysis/emit/Overlay5DrawPool_InitializeStatic.o
 */
#include <game/overlay005_pools.h>
#include <msl/object_register.h>
extern "C" {
extern Overlay5DrawPool data_ov005_0206a3f8;
extern MslDestructorNode data_ov005_0206a3ec;
Overlay5DrawPool *func_ov005_020692a0(Overlay5DrawPool *);
}
#pragma define_section SceneInit ".init" abs32 RX
#pragma section SceneInit begin
void Overlay5DrawPool_InitializeStatic(void)
{
    data_ov005_0206a3f8.entries = 0;
    data_ov005_0206a3f8.links = 0;
    data_ov005_0206a3f8.free_list = 0;
    data_ov005_0206a3f8.taken_list = 0;
    data_ov005_0206a3f8.count = 0;
    MSL_RegisterGlobalObject(&data_ov005_0206a3f8, (MslGlobalDestructor)func_ov005_020692a0, &data_ov005_0206a3ec);
}
#pragma section SceneInit end