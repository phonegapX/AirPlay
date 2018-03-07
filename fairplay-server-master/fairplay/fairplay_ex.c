#include "fairplay.h"
#include "remap_segment.h"

void binary___t_start(){
	;
}


int __cdecl call_0xeb00c(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
	unsigned __int8 v7; // al@5
	char v8; // al@46
	unsigned __int8 v9; // al@49
	int v10; // ST34_4@63
	int v11; // ST2C_4@63
	unsigned __int8 v12; // al@70
	int v13; // ST2C_4@77
	int v15; // ST2C_4@85
	int v16; // ST2C_4@86
	int v17; // ST2C_4@87
	int v18; // ST48_4@88
	int v19; // ST2C_4@88
	int v20; // [sp+1Ch] [bp-50h]@5
	int v21; // [sp+1Ch] [bp-50h]@57
	int v22; // [sp+1Ch] [bp-50h]@70
	signed int v23; // [sp+20h] [bp-4Ch]@57
	int v24; // [sp+24h] [bp-48h]@7
	signed int v25; // [sp+24h] [bp-48h]@9
	int v26; // [sp+24h] [bp-48h]@38
	signed int v27; // [sp+24h] [bp-48h]@40
	int v28; // [sp+24h] [bp-48h]@51
	int v29; // [sp+24h] [bp-48h]@72
	unsigned int v30; // [sp+28h] [bp-44h]@3
	int v31; // [sp+28h] [bp-44h]@5
	int v32; // [sp+28h] [bp-44h]@12
	unsigned int v33; // [sp+28h] [bp-44h]@13
	int v34; // [sp+28h] [bp-44h]@15
	int v35; // [sp+28h] [bp-44h]@17
	int v36; // [sp+28h] [bp-44h]@25
	unsigned int v37; // [sp+28h] [bp-44h]@26
	signed int v38; // [sp+28h] [bp-44h]@28
	int v39; // [sp+28h] [bp-44h]@32
	int v40; // [sp+28h] [bp-44h]@34
	int v41; // [sp+28h] [bp-44h]@36
	signed int v42; // [sp+28h] [bp-44h]@44
	signed int v43; // [sp+28h] [bp-44h]@48
	int v44; // [sp+28h] [bp-44h]@49
	unsigned int v45; // [sp+28h] [bp-44h]@55
	int v46; // [sp+28h] [bp-44h]@70
	int v47; // [sp+28h] [bp-44h]@78
	int v48; // [sp+2Ch] [bp-40h]@5
	int v49; // [sp+2Ch] [bp-40h]@7
	signed int v50; // [sp+2Ch] [bp-40h]@17
	signed int v51; // [sp+2Ch] [bp-40h]@19
	signed int v52; // [sp+2Ch] [bp-40h]@28
	signed int v53; // [sp+2Ch] [bp-40h]@32
	signed int v54; // [sp+2Ch] [bp-40h]@34
	int v55; // [sp+2Ch] [bp-40h]@36
	int v56; // [sp+2Ch] [bp-40h]@38
	int v57; // [sp+2Ch] [bp-40h]@49
	int v58; // [sp+2Ch] [bp-40h]@70
	int v59; // [sp+30h] [bp-3Ch]@7
	int v60; // [sp+30h] [bp-3Ch]@17
	int v61; // [sp+30h] [bp-3Ch]@38
	int v62; // [sp+30h] [bp-3Ch]@72
	int v63; // [sp+34h] [bp-38h]@9
	int v64; // [sp+34h] [bp-38h]@17
	int v65; // [sp+34h] [bp-38h]@40
	int v66; // [sp+34h] [bp-38h]@74
	signed int v67; // [sp+38h] [bp-34h]@7
	int v68; // [sp+38h] [bp-34h]@17
	signed int v69; // [sp+38h] [bp-34h]@38
	signed int v70; // [sp+38h] [bp-34h]@51
	signed int v71; // [sp+38h] [bp-34h]@72
	signed int v72; // [sp+38h] [bp-34h]@74
	int v73; // [sp+44h] [bp-28h]@1

	binary___t_start();
	v73 = make_stack(18);
	writeMem_int(v73 + 20, v73 + 12);
	writeMem_int(v73 + 16, v73 + 8);
	writeMem_int(567205364, v73 + 20);
	writeMem_int(567205367, v73 + 16);
	writeMem_int(567205368, v73 + 4);
	writeMem_int(a2, v73);
LABEL_3:
	while ( 2 )
	{
		v30 = readMem_int(v73 + 4) - 567205353;
		if ( v30 > 0x10 )
			goto LABEL_25;
		switch ( v30 )
		{
		default:
			v7 = readMem_char(a4 + 7);
			v31 = v7;
			v20 = v7;
			v48 = v73 + 16;
			if ( !v7 )
				v48 = v73 + 20;
			v49 = readMem_int(v48);
			v67 = 1392463288;
			v24 = readMem_int(v73 + 12);
			v59 = readMem_int(v73 + 8);
			if ( !v20 )
				v67 = -107773803;
			v63 = readMem_int(v73 + 4);
			writeMem_int(v63 + v67, v24);
			v25 = 1283023906;
			if ( !v31 )
				v25 = 433002499;
			writeMem_int(v63 + v25, v59);
			writeMem_int(v49, v73 + 4);
			if ( v31 )
				goto LABEL_12;
			goto LABEL_25;
		case 0x10u:
			goto LABEL_12;
		case 0xEu:
			goto LABEL_15;
		case 0xFu:
			goto LABEL_28;
		case 0u:
LABEL_44:
			v42 = 1048900629;
			writeMem_int(1048900629, v73 + 20);
			goto LABEL_45;
		case 4u:
			v9 = readMem_char(a4 + 6);
			writeMem_char((127 - ((2 * v9 & 0xEE) + (v9 ^ 0xF7) + 9)) ^ 0x7F, v73 + 32);
			writeMem_int(a4, v73 + 36);
			call_0xeb9b4(v73 + 24);
			v44 = readMem_int(v73 + 28);
			v57 = v73 + 16;
			if ( !v44 )
				v57 = v73 + 20;
			v56 = readMem_int(v57);
			v70 = 491530432;
			v28 = readMem_int(v73 + 12);
			v61 = readMem_int(v73 + 8);
			if ( !v44 )
				v70 = 1420803416;
			v65 = readMem_int(v73 + 4);
			writeMem_int(v65 + v70, v28);
			v27 = -410217175;
			if ( !v44 )
				v27 = 216864114;
			goto LABEL_42;
		case 0xDu:
			v45 = (unsigned __int8)readMem_char(a4 + 6) - 1;
			if ( v45 > 3 )
			{
LABEL_65:
				v43 = 567205359;
				writeMem_int(567205359, v73 + 20);
				goto LABEL_61;
			}
			if ( v45 == 1 )
			{
				while ( 2 )
				{
					v42 = 1048900632;
					writeMem_int(1048900632, v73 + 20);
LABEL_45:
					writeMem_int(v42, v73 + 4);
					do
					{
LABEL_12:
						v32 = readMem_int(v73 + 4);
						if ( v32 <= 1048900628 )
						{
							if ( v32 == 567205370 )
								goto LABEL_25;
							goto LABEL_3;
						}
						v33 = v32 - 1048900629;
						if ( v33 > 6 )
							goto LABEL_3;
						switch ( v33 )
						{
						case 1u:
							goto LABEL_60;
						case 2u:
							goto LABEL_65;
						case 5u:
							v12 = readMem_char(a4 + 5);
							v46 = v12;
							v22 = v12;
							writeMem_int(-42086, v73 + 64);
							v58 = v73 + 16;
							if ( v22 == 1 )
								v58 = v73 + 20;
							v56 = readMem_int(v58);
							v71 = 205134838;
							v62 = readMem_int(v73 + 8);
							v29 = readMem_int(v73 + 12);
							if ( v22 == 1 )
								v71 = 660018200;
							v66 = readMem_int(v73 + 4);
							writeMem_int(v66 + v71, v29);
							v72 = 476232582;
							if ( v46 == 1 )
								v72 = 1;
							writeMem_int(v66 + v72, v62);
							goto LABEL_43;
						case 6u:
							writeMem_char(-1, a7);
							v47 = (unsigned __int8)readMem_char(a4 + 7);
							goto LABEL_79;
						case 3u:
							writeMem_int(a1, v73 + 28);
							v16 = readMem_int(v73);
							writeMem_int(v16, v73 + 52);
							writeMem_int(a3, v73 + 32);
							writeMem_int(a4, v73 + 36);
							writeMem_int(a5, v73 + 40);
							writeMem_int(a6, v73 + 56);
							writeMem_int(a7, v73 + 48);
							call_0x6787c(v73 + 24);
							v47 = readMem_int(v73 + 44);
							goto LABEL_79;
						case 4u:
							v18 = readMem_int(v73 + 68);
							writeMem_int(a1, v73 + 48);
							v19 = readMem_int(v73);
							writeMem_int(v19, v73 + 28);
							writeMem_int(v18, v73 + 36);
							writeMem_int(a4, v73 + 32);
							writeMem_int(a5, v73 + 40);
							writeMem_int(a6, v73 + 52);
							writeMem_int(a7, v73 + 44);
							call_0x6712c(v73 + 24);
							v47 = readMem_int(v73 + 56);
							goto LABEL_79;
						case 0u:
							goto LABEL_90;
						default:
							break;
						}
LABEL_15:
						v34 = v73 + 16;
						if ( !a4 )
							v34 = v73 + 20;
						v35 = readMem_int(v34);
						v50 = -506455848;
						v64 = readMem_int(v73 + 4);
						v68 = readMem_int(v73 + 8);
						v60 = readMem_int(v73 + 12);
						if ( a4 )
							v50 = -2;
						writeMem_int(v64 + v50, v60);
						v51 = -13198209;
						if ( a4 )
							v51 = -3;
						writeMem_int(v64 + v51, v68);
						writeMem_int(v35, v73 + 4);
					}
					while ( a4 );
					do
					{
LABEL_25:
						while ( 1 )
						{
							v36 = readMem_int(v73 + 4);
							if ( v36 > 1708918828 )
								break;
							if ( v36 == 567205369 )
								goto LABEL_12;
							if ( v36 == 1048900636 )
								goto LABEL_3;
						}
						v37 = v36 - 1708918829;
					}
					while ( v37 > 5 );
					switch ( v37 )
					{
					case 5u:
						v8 = readMem_char(a4 + 4);
						if ( v8 == 2 )
							goto LABEL_62;
						if ( v8 != 3 )
							goto LABEL_48;
						goto LABEL_64;
					case 0u:
LABEL_48:
						v43 = 567205356;
						writeMem_int(567205356, v73 + 20);
						goto LABEL_61;
					case 4u:
						v21 = a1;
						v23 = 2;
						if ( a1 != 2 )
						{
							v21 = a1;
							v23 = 3;
						}
						if ( v21 == v23 )
							goto LABEL_60;
						goto LABEL_44;
					case 1u:
LABEL_64:
						writeMem_int(1708918830, v73 + 4);
						goto LABEL_63;
					case 2u:
						continue;
					case 3u:
						goto LABEL_68;
					default:
						goto LABEL_28;
					}
				}
LABEL_28:
				writeMem_int(a3, v73 + 68);
				writeMem_int(-42023, v73 + 64);
				v52 = 0;
				v38 = 0;
				if ( !a5 )
					v38 = 1;
				if ( !a3 )
					v52 = 1;
				v39 = v52 | v38;
				v53 = 0;
				if ( !a6 )
					v53 = 1;
				v40 = v53 | v39;
				v54 = 0;
				if ( !a7 )
					v54 = 1;
				v41 = v54 | v40;
				v55 = v73 + 16;
				if ( v41 )
					v55 = v73 + 20;
				v56 = readMem_int(v55);
				v69 = 1141713465;
				v26 = readMem_int(v73 + 12);
				v61 = readMem_int(v73 + 8);
				if ( v41 )
					v69 = -337184955;
				v65 = readMem_int(v73 + 4);
				writeMem_int(v65 + v69, v26);
				v27 = 481695266;
				if ( v41 )
					v27 = 514262061;
LABEL_42:
				writeMem_int(v65 + v27, v61);
LABEL_43:
				writeMem_int(v56, v73 + 4);
				continue;
			}
			if ( v45 < 1 )
			{
LABEL_66:
				v43 = 567205360;
				writeMem_int(567205360, v73 + 20);
				goto LABEL_61;
			}
			if ( v45 == 2 )
			{
LABEL_68:
				v43 = 567205362;
				writeMem_int(567205362, v73 + 20);
LABEL_61:
				writeMem_int(v43, v73 + 4);
				continue;
			}
			if ( v45 == 3 )
			{
LABEL_69:
				v42 = 1048900633;
				writeMem_int(1048900633, v73 + 20);
				goto LABEL_45;
			}
LABEL_85:
			writeMem_int(a1, v73 + 52);
			v15 = readMem_int(v73);
			writeMem_int(v15, v73 + 28);
			writeMem_int(a3, v73 + 44);
			writeMem_int(a4, v73 + 36);
			writeMem_int(a5, v73 + 40);
			writeMem_int(a6, v73 + 32);
			writeMem_int(a7, v73 + 56);
			call_0xf5660(v73 + 24);
			v47 = readMem_int(v73 + 48);
LABEL_79:
			pop_stack();
			return v47;
		case 2u:
LABEL_60:
			v43 = 567205354;
			writeMem_int(567205354, v73 + 20);
			goto LABEL_61;
		case 5u:
LABEL_62:
			writeMem_int(567205358, v73 + 4);
LABEL_63:
			v10 = readMem_int(v73 + 12);
			writeMem_int(567205357, v73 + 20);
			v11 = readMem_int(v73 + 8);
			writeMem_int(567205366, v10);
			writeMem_int(567205356, v11);
			writeMem_int(567205357, v73 + 4);
			continue;
		case 8u:
			goto LABEL_66;
		case 0xAu:
			goto LABEL_69;
		case 1u:
			writeMem_int(a1, v73 + 36);
			v13 = readMem_int(v73);
			writeMem_int(v13, v73 + 48);
			writeMem_int(a3, v73 + 40);
			writeMem_int(a4, v73 + 44);
			writeMem_int(a5, v73 + 52);
			writeMem_int(a6, v73 + 56);
			writeMem_int(a7, v73 + 28);
			call_0xcf77c(v73 + 24);
			goto LABEL_78;
		case 7u:
			goto LABEL_85;
		case 9u:
			writeMem_int(a1, v73 + 56);
			v17 = readMem_int(v73);
			writeMem_int(v17, v73 + 36);
			writeMem_int(a3, v73 + 40);
			writeMem_int(a4, v73 + 44);
			writeMem_int(a5, v73 + 52);
			writeMem_int(a6, v73 + 48);
			writeMem_int(a7, v73 + 28);
			call_0xa2114(v73 + 24);
LABEL_78:
			v47 = readMem_int(v73 + 32);
			goto LABEL_79;
		case 6u:
			writeMem_char(-1, a7);
			v47 = -42081;
			goto LABEL_79;
		case 3u:
LABEL_90:
			writeMem_char(-1, a7);
			v47 = -42091;
			goto LABEL_79;
		case 0xBu:
			v47 = readMem_int(v73 + 64);
			goto LABEL_79;
		}
	}
}