/* Overlay5ItemPool_InitializeStatic (ov005, 0x02069E24-0x02069EB8).
 * Binary-matched implementation in build/analysis/emit/Overlay5ItemPool_InitializeStatic.o
 */
#include <game/overlay005_pools.h>
#include <msl/object_register.h>
extern "C" {
extern Overlay5ItemPool data_ov005_0206aa18;
extern MslDestructorNode data_ov005_0206aa0c;
Overlay5ItemPool *func_ov005_020698b0(Overlay5ItemPool *);
}
#pragma define_section SceneInit ".init" abs32 RX
#pragma section SceneInit begin
void Overlay5ItemPool_InitializeStatic(void)
{
    data_ov005_0206aa18.items = 0;
    data_ov005_0206aa18.links = 0;
    data_ov005_0206aa18.free_list = 0;
    data_ov005_0206aa18.taken_list = 0;
    data_ov005_0206aa18.count = 0;
    MSL_RegisterGlobalObject(&data_ov005_0206aa18, (MslGlobalDestructor)func_ov005_020698b0, &data_ov005_0206aa0c);
}
#pragma section SceneInit end