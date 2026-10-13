/* Overlay5ElementPool_InitializeStatic (ov005, 0x02069D6C-0x02069DAC).
 * Binary-matched implementation in build/analysis/emit/Overlay5ElementPool_InitializeStatic.o
 */
#include <game/overlay005_pools.h>
#include <msl/object_register.h>
extern "C" {
extern Overlay5ElementPool data_ov005_0206a240;
extern MslDestructorNode data_ov005_0206a234;
Overlay5ElementPool *func_ov005_02066864(Overlay5ElementPool *);
}
#pragma define_section SceneInit ".init" abs32 RX
#pragma section SceneInit begin
void Overlay5ElementPool_InitializeStatic(void)
{
    data_ov005_0206a240.elements = 0;
    data_ov005_0206a240.links = 0;
    data_ov005_0206a240.free_list = 0;
    data_ov005_0206a240.taken_list = 0;
    data_ov005_0206a240.count = 0;
    MSL_RegisterGlobalObject(&data_ov005_0206a240, (MslGlobalDestructor)func_ov005_02066864, &data_ov005_0206a234);
}
#pragma section SceneInit end