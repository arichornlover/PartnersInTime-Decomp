/* func_ov006_0206e594 (ov006, 0x0206E594-0x0206E644).
 * Binary-matched implementation in build/analysis/emit/func_ov006_0206e594.o
 */
#include <game/battle_scene_object.h>
extern "C" {
extern int _s32_div_f(int, int);
BattleSceneObject *BattleSceneObject_BeginMotionChannel(BattleSceneObject *, int, int, int, int);
BattleSceneObject *BattleSceneObject_GetActiveModel(BattleSceneObject *);
}
void func_ov006_0206e594(BattleSceneObject *obj, int arg)
{
    BattleSceneObject *model = BattleSceneObject_GetActiveModel(obj);
    int val = model->field_4;
    short sval = model->field_8;
    int shifted = val >> 7;
    int tmp = val + (shifted >> 24);
    tmp = tmp >> 8;
    if (tmp >= sval) {
        obj->field_74 = 0x100;
        obj->field_76 = 0x100;
        return;
    }
    int div_result = _s32_div_f(tmp << 16, 0);
    short div_shifted = div_result >> 4;
    extern short data_ov006_0207380c[]; // literal pool
    short table_val = data_ov006_0207380c[div_shifted << 2];
    short sval2 = *(short *)((char *)model + 0x18);
    int mul = (table_val >> 3) * sval2;
    mul = mul + (mul >> 28);
    mul = mul >> 4;
    int final = mul >> 7;
    final = final + (final >> 24);
    final = final >> 8;
    int result = final + 0x100;
    obj->field_74 = result;
    obj->field_76 = 0x100 - final;
}