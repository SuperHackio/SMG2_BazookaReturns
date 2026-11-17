# SMG2 BazookaReturns

## Undergrunt Gunner in SMG2

Undergrunt Gunners return in SMG2!

Below is a list of everything that's included:
- The WaterBazooka class for ProductMapObjDataTable.bcsv.
- The JSON files for particle effects.

Below is a list of other things that you will need:
- A level editor.
- A particle editor with support for JSON files.
- ActionSound entries with all sound labels for BossBazooka.

## Credits
- [@Bavario](https://github.com/bavario-lginc) - Initial porting efforts.
- [@MrL314](https://github.com/MrL314) - Decompilation.
- [@Super Hackio](https://github.com/SuperHackio) - Ported and enhanced SMG1 code to SMG2.

## Setup
It is adviced to reference the [template mod](https://youtu.be/KBOr-3MzRdc) for this port. You can use the following parameters:

### WaterBazooka
| Parameter | Description |
| --- | --- |
| Obj_arg0 | Power Star ID. Spawns the assigned Power Star when defeated. Optional. |
| Obj_arg1 | AlreadyDoneFlag. If set, the Undergrunt will stay defeated upon respawn, like after dying. This will also activate SW_DEAD. Does not apply anymore when the level is exited. |
| Obj_arg2 | PinchBGM. Makes the Undergrunt Gunner use the [PinchBGM system](https://github.com/SuperHackio/SMG2_PinchBgm). |
| Obj_arg3 | Riddle SFX. If set, it will play `SE_SY_READ_RIDDLE_S` after being defeated. |
| SW_DEAD | Gets activated when defeated. |
| SW_A | Starts the fight when activated. |
| SW_B | Gets activated in the 3rd phase of the fight (after the 2nd blow). |
> *Note: Custom particles can be defined via InitActor.bcsv. The projectiles and ActionSound triggers however are defined via the object name used in the editor. WaterBazooka will use bubble projectiles (default), ElectricBazooka will use electric ball projectiles and FireBazooka will use fireball projectiles.*
