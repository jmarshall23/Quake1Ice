// game_api.cpp
//

#include "../quakedef.h"
#include "game_local.h"

extern	cvar_t	sv_aim;

namespace QuakeAPI
{

	void SetMinMaxSize(edict_t* e, const vec3_t& min, const vec3_t& max, qboolean rotate)
	{
		float* angles;
		vec3_t	rmin, rmax;
		float	bounds[2][3];
		float	xvector[2], yvector[2];
		float	a;
		vec3_t	base, transformed;
		int		i, j, k, l;

		for (i = 0; i < 3; i++)
			if (min[i] > max[i])
				PR_RunError("backwards mins/maxs");

		rotate = false;		// FIXME: implement rotation properly again

		if (!rotate)
		{
			VectorCopy(min, rmin);
			VectorCopy(max, rmax);
		}
		else
		{
			// find min / max for rotations
			angles = e->v.angles;

			a = angles[1] / 180 * M_PI;

			xvector[0] = cos(a);
			xvector[1] = sin(a);
			yvector[0] = -sin(a);
			yvector[1] = cos(a);

			VectorCopy(min, bounds[0]);
			VectorCopy(max, bounds[1]);

			rmin[0] = rmin[1] = rmin[2] = 9999;
			rmax[0] = rmax[1] = rmax[2] = -9999;

			for (i = 0; i <= 1; i++)
			{
				base[0] = bounds[i][0];
				for (j = 0; j <= 1; j++)
				{
					base[1] = bounds[j][1];
					for (k = 0; k <= 1; k++)
					{
						base[2] = bounds[k][2];

						// transform the point
						transformed[0] = xvector[0] * base[0] + yvector[0] * base[1];
						transformed[1] = xvector[1] * base[0] + yvector[1] * base[1];
						transformed[2] = base[2];

						for (l = 0; l < 3; l++)
						{
							if (transformed[l] < rmin[l])
								rmin[l] = transformed[l];
							if (transformed[l] > rmax[l])
								rmax[l] = transformed[l];
						}
					}
				}
			}
		}

		// set derived values
		VectorCopy(rmin, e->v.mins);
		VectorCopy(rmax, e->v.maxs);
		VectorSubtract(max, min, e->v.size);

		SV_LinkEdict(e, false);
	}

	void makevectors(const vec3_t & org)
	{
		AngleVectors(org, pr_global_struct->v_forward, pr_global_struct->v_right, pr_global_struct->v_up);
	}

	void setorigin(edict_t* e, const vec3_t & org)
	{
		VectorCopy(org, e->v.origin);
		SV_LinkEdict(e, false);
	}

	void setsize(edict_t* e, const vec3_t & min, const vec3_t & max)
	{
		SetMinMaxSize(e, min, max, false);
	}

	void setmodel(edict_t* e, const char* m)
	{
		model_t* mod;
		int i;

		for (i = 0; sv.model_precache[i]; i++)
			if (!strcmp(sv.model_precache[i], m))
				break;

		if (!sv.model_precache[i])
			PR_RunError("no precache: %s\n", m);

		e->v.model = m - pr_strings;
		e->v.modelindex = i;

		mod = sv.models[(int)e->v.modelindex];

		if (mod)
			SetMinMaxSize(e, mod->mins, mod->maxs, true);
		else
			SetMinMaxSize(e, vec3_origin, vec3_origin, true);
	}

	void bprint(const char* s)
	{
		SV_BroadcastPrintf("%s", s);
	}

	void sprint(int entnum, const char* s)
	{
		if (entnum < 1 || entnum > svs.maxclients)
		{
			Con_Printf("tried to sprint to a non-client\n");
			return;
		}

		client_t* client = &svs.clients[entnum - 1];

		MSG_WriteChar(&client->message, svc_print);
		MSG_WriteString(&client->message, (char *)s);
	}

	void centerprint(int entnum, const char* s)
	{
		if (entnum < 1 || entnum > svs.maxclients)
		{
			Con_Printf("tried to centerprint to a non-client\n");
			return;
		}

		client_t* client = &svs.clients[entnum - 1];

		MSG_WriteChar(&client->message, svc_centerprint);
		MSG_WriteString(&client->message, (char*)s);
	}

	vec3_t normalize(const vec3_t & value1)
	{
		vec3_t newvalue;
		float length = sqrt(value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2]);

		if (length == 0)
			newvalue[0] = newvalue[1] = newvalue[2] = 0;
		else
		{
			length = 1 / length;
			newvalue[0] = value1[0] * length;
			newvalue[1] = value1[1] * length;
			newvalue[2] = value1[2] * length;
		}

		return newvalue;
	}

	float vlen(const vec3_t & value1)
	{
		return sqrt(value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2]);
	}

	float vectoyaw(const vec3_t & value1)
	{
		if (value1[1] == 0 && value1[0] == 0)
			return 0;
		else
		{
			float yaw = atan2(value1[1], value1[0]) * 180 / M_PI;
			return yaw < 0 ? yaw + 360 : yaw;
		}
	}

	vec3_t vectoangles(const vec3_t & value1)
	{
		vec3_t angles;
		float forward;
		if (value1[1] == 0 && value1[0] == 0)
		{
			angles[0] = value1[2] > 0 ? 90 : 270;
			angles[1] = 0;
		}
		else
		{
			angles[1] = atan2(value1[1], value1[0]) * 180 / M_PI;
			if (angles[1] < 0) angles[1] += 360;
			forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
			angles[0] = atan2(value1[2], forward) * 180 / M_PI;
			if (angles[0] < 0) angles[0] += 360;
		}
		angles[2] = 0;
		return angles;
	}

	float random()
	{
		return (rand() & 0x7fff) / (float)0x7fff;
	}

	void particle(const vec3_t & org, const vec3_t & dir, float color, float count)
	{
		SV_StartParticle(org, dir, color, count);
	}

	void ambientsound(const vec3_t & pos, const char* samp, float vol, float attenuation)
	{
		int i, soundnum;

		for (soundnum = 0; sv.sound_precache[soundnum]; soundnum++)
			if (!strcmp(sv.sound_precache[soundnum], samp))
				break;

		if (!sv.sound_precache[soundnum])
		{
			Con_Printf("no precache: %s\n", samp);
			return;
		}

		MSG_WriteByte(&sv.signon, svc_spawnstaticsound);
		for (i = 0; i < 3; i++)
			MSG_WriteCoord(&sv.signon, pos[i]);

		MSG_WriteByte(&sv.signon, soundnum);
		MSG_WriteByte(&sv.signon, vol * 255);
		MSG_WriteByte(&sv.signon, attenuation * 64);
	}

	void sound(edict_t* entity, int channel, const char* sample, int volume, float attenuation)
	{
		if (volume < 0 || volume > 255)
			Sys_Error("SV_StartSound: volume = %i", volume);

		if (attenuation < 0 || attenuation > 4)
			Sys_Error("SV_StartSound: attenuation = %f", attenuation);

		if (channel < 0 || channel > 7)
			Sys_Error("SV_StartSound: channel = %i", channel);

		SV_StartSound(entity, channel, (char *)sample, volume, attenuation);
	}

	void break_statement()
	{
		Con_Printf("break statement\n");
		// Causes a deliberate crash for debugging.
		*(int*)-4 = 0;
	}

	void traceline(const vec3_t & v1, const vec3_t & v2, int nomonsters, edict_t* ent)
	{
		trace_t trace = SV_Move(v1, vec3_origin, vec3_origin, v2, nomonsters, ent);

		pr_global_struct->trace_allsolid = trace.allsolid;
		pr_global_struct->trace_startsolid = trace.startsolid;
		pr_global_struct->trace_fraction = trace.fraction;
		pr_global_struct->trace_inwater = trace.inwater;
		pr_global_struct->trace_inopen = trace.inopen;
		VectorCopy(trace.endpos, pr_global_struct->trace_endpos);
		VectorCopy(trace.plane.normal, pr_global_struct->trace_plane_normal);
		pr_global_struct->trace_plane_dist = trace.plane.dist;
		if (trace.ent)
			pr_global_struct->trace_ent = EDICT_TO_PROG(trace.ent);
		else
			pr_global_struct->trace_ent = EDICT_TO_PROG(sv.edicts);
	}


	byte	checkpvs[MAX_MAP_LEAFS / 8];

	int newcheckclient(int check)
	{
		int		i;
		byte* pvs;
		edict_t* ent;
		mleaf_t* leaf;
		vec3_t	org;

		// cycle to the next one

		if (check < 1)
			check = 1;
		if (check > svs.maxclients)
			check = svs.maxclients;

		if (check == svs.maxclients)
			i = 1;
		else
			i = check + 1;

		for (; ; i++)
		{
			if (i == svs.maxclients + 1)
				i = 1;

			ent = EDICT_NUM(i);

			if (i == check)
				break;	// didn't find anything else

			if (ent->free)
				continue;
			if (ent->v.health <= 0)
				continue;
			if ((int)ent->v.flags & FL_NOTARGET)
				continue;

			// anything that is a client, or has a client as an enemy
			break;
		}

		// get the PVS for the entity
		VectorAdd(ent->v.origin, ent->v.view_ofs, org);
		leaf = Mod_PointInLeaf(org, sv.worldmodel);
		pvs = Mod_LeafPVS(leaf, sv.worldmodel);
		memcpy(checkpvs, pvs, (sv.worldmodel->numleafs + 7) >> 3);

		return i;
	}

	edict_t* checkclient(void)
	{
		edict_t* ent, * self;
		mleaf_t* leaf;
		int l;
		vec3_t view;

		if (sv.time - sv.lastchecktime >= 0.1)
		{
			sv.lastcheck = newcheckclient(sv.lastcheck);
			sv.lastchecktime = sv.time;
		}

		ent = EDICT_NUM(sv.lastcheck);
		if (ent->free || ent->v.health <= 0)
		{
			return sv.edicts;
		}

		self = PROG_TO_EDICT(pr_global_struct->self);
		VectorAdd(self->v.origin, self->v.view_ofs, view);
		leaf = Mod_PointInLeaf(view, sv.worldmodel);
		l = (leaf - sv.worldmodel->leafs) - 1;
		if ((l < 0) || !(checkpvs[l >> 3] & (1 << (l & 7))))
		{
			return sv.edicts;
		}

		return ent;
	}

	void stuffcmd(edict_t* ent, const char* str)
	{
		if (ent->entnum < 1 || ent->entnum > svs.maxclients)
		{
			PR_RunError("Parm 0 not a client");
		}

		client_t* old = host_client;
		host_client = &svs.clients[ent->entnum - 1];
		Host_ClientCommands("%s", str);
		host_client = old;
	}

	void localcmd(const char* str)
	{
		Cbuf_AddText((char*)str);
	}

	float cvar(const char* str)
	{
		return Cvar_VariableValue((char*)str);
	}

	void cvar_set(const char* var, const char* val)
	{
		Cvar_Set((char *)var, (char*)val);
	}

	bool checkpos(edict_t* entity, const vec3_t & vector)
	{
		// Implementation needed
		return false; // Placeholder return
	}

	edict_t* PF_findradius(const vec3_t & org, float rad)
	{
		edict_t* ent, * chain;
		vec3_t eorg;
		int i, j;

		chain = (edict_t*)sv.edicts;

		ent = NEXT_EDICT(sv.edicts);
		for (i = 1; i < sv.num_edicts; i++, ent = NEXT_EDICT(ent))
		{
			if (ent->free || ent->v.solid == SOLID_NOT)
				continue;

			for (j = 0; j < 3; j++)
				eorg[j] = org[j] - (ent->v.origin[j] + (ent->v.mins[j] + ent->v.maxs[j]) * 0.5);

			if (Length(eorg) > rad)
				continue;

			ent->v.chain = EDICT_TO_PROG(chain);
			chain = ent;
		}

		return chain;
	}

	void dprint(const char* str) {
		Con_DPrintf("%s", str);
	}

	std::string ftos(float v) {
		char temp[128];
		if (v == static_cast<int>(v))
			sprintf(temp, "%d", static_cast<int>(v));
		else
			sprintf(temp, "%5.1f", v);
		return std::string(temp);
	}

	std::string vtos(const vec3_t & v) {
		char temp[128];
		sprintf(temp, "'%5.1f %5.1f %5.1f'", v.x, v.y, v.z);
		return std::string(temp);
	}

	std::string etos(edict_t* e) {
		char temp[128];
		sprintf(temp, "entity %i", e - sv.edicts); // Assuming sv.edicts is the start of the edicts array
		return std::string(temp);
	}

	edict_t* Spawn(void) {
		return ED_Alloc();
	}

	void Remove(edict_t* ed) {
		ED_Free(ed);
	}

	edict_t* Find(edict_t* start, int field, const char* match) {
		int e = NUM_FOR_EDICT(start) + 1;
		for (; e < sv.num_edicts; e++) {
			edict_t* ed = EDICT_NUM(e);
			if (ed->free) continue;
			char* t = E_STRING(ed, field);
			if (!t) continue;
			if (!strcmp(t, match)) {
				return ed;
			}
		}
		return sv.edicts; // return world if not found
	}


	void CheckEmptyString(const char* s) {
		if (s[0] <= ' ') {
			PR_RunError("Bad string");
		}
	}

	int precache_file(const char* filename) {
		// In a real implementation, you would cache the file here.
		// For this stub, just pretend we did.
		return 0; // Assuming 0 is the index of the filename in the precache list
	}

	int precache_sound(const char* soundname) {
		CheckEmptyString(soundname);
		if (sv.state != ss_loading) {
			PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");
		}
		for (int i = 0; i < MAX_SOUNDS; i++) {
			if (!sv.sound_precache[i]) {
				sv.sound_precache[i] = (char *)soundname;
				return i;
			}
			if (!strcmp(sv.sound_precache[i], soundname)) {
				return i;
			}
		}
		PR_RunError("PF_precache_sound: overflow");
		return 0; // To satisfy compiler, actual execution should never reach here.
	}

	int precache_model(const char* modelname) {
		CheckEmptyString(modelname);
		if (sv.state != ss_loading) {
			PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");
		}
		for (int i = 0; i < MAX_MODELS; i++) {
			if (!sv.model_precache[i]) {
				sv.model_precache[i] = (char *)modelname;
				sv.models[i] = Mod_ForName((char*)modelname, true);
				return i;
			}
			if (!strcmp(sv.model_precache[i], modelname)) {
				return i;
			}
		}
		PR_RunError("PF_precache_model: overflow");
		return 0; // To satisfy compiler, actual execution should never reach here.
	}

	void coredump()
	{
		ED_PrintEdicts();
	}

	void traceon()
	{
		pr_trace = true;
	}

	void traceoff()
	{
		pr_trace = false;
	}

	void eprint(edict_t* e0)
	{
		//ED_PrintNum(EDICT_NUM(e0));
	}

	float walkmove(float yaw, float dist)
	{
		edict_t* ent;
		vec3_t move;
		dfunction_t* oldf;
		int oldself;

		ent = PROG_TO_EDICT(pr_global_struct->self);
		yaw = yaw * M_PI * 2 / 360;

		move[0] = cos(yaw) * dist;
		move[1] = sin(yaw) * dist;
		move[2] = 0;

		// save program state, because SV_movestep may call other progs
		oldf = pr_xfunction;
		oldself = pr_global_struct->self;

		float result = SV_movestep(ent, move, true);

		// restore program state
		pr_xfunction = oldf;
		pr_global_struct->self = oldself;

		return result;
	}

	int droptofloor()
	{
		edict_t* ent;
		vec3_t end;
		trace_t trace;

		ent = PROG_TO_EDICT(pr_global_struct->self);

		VectorCopy(ent->v.origin, end);
		end[2] -= 256;

		trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

		if (trace.fraction == 1 || trace.allsolid)
			return 0;
		else
		{
			VectorCopy(trace.endpos, ent->v.origin);
			SV_LinkEdict(ent, false);
			ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
			ent->v.groundentity = EDICT_TO_PROG(trace.ent);
			return 1;
		}
	}

	void lightstyle(float style, const char* s0)
	{
		int j;
		client_t* client;

		// change the string in sv
		sv.lightstyles[(int)style] = (char *)s0;

		// send message to all clients on this server
		if (sv.state != ss_active)
			return;

		for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
			if (client->active || client->spawned)
			{
				MSG_WriteChar(&client->message, svc_lightstyle);
				MSG_WriteChar(&client->message, style);
				MSG_WriteString(&client->message, (char*)s0);
			}
	}

	int rint(float f0)
	{
		if (f0 > 0)
			return (int)(f0 + 0.5);
		else
			return (int)(f0 - 0.5);
	}

	void floor(float f0)
	{
		return floor(f0);
	}

	void ceil(float f0)
	{
		return ceil(f0);
	}

	int checkbottom(edict_t* e0)
	{
		return SV_CheckBottom(e0);
	}

	int pointcontents(vec3_t p0)
	{
		return SV_PointContents(p0);
	}

	edict_t* nextent(edict_t* e0)
	{
		int i;
		edict_t* ent;

		i = NUM_FOR_EDICT(e0);
		while (1)
		{
			i++;
			if (i == sv.num_edicts)
			{
				return sv.edicts; // Equivalent to RETURN_EDICT(sv.edicts);
			}
			ent = EDICT_NUM(i);
			if (!ent->free)
			{
				return ent; // Equivalent to RETURN_EDICT(ent);
			}
		}
	}

	vec3_t aim(edict_t* ent, float speed)
	{
		vec3_t start, dir, end, bestdir;
		edict_t* check, * bestent = nullptr;
		trace_t tr;
		float dist, bestdist = sv_aim.value;
		int i;

		VectorCopy(ent->v.origin, start);
		start[2] += 20;

		// try sending a trace straight
		VectorCopy(pr_global_struct->v_forward, dir);
		VectorMA(start, 2048, dir, end);
		tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
		if (tr.ent && tr.ent->v.takedamage == DAMAGE_AIM && (!teamplay.value || ent->v.team <= 0 || ent->v.team != tr.ent->v.team)) {
			VectorCopy(pr_global_struct->v_forward, dir);
			return dir;
		}

		// try all possible entities
		VectorCopy(dir, bestdir);
		bestent = nullptr;

		for (i = 1; i < sv.num_edicts; i++) {
			check = EDICT_NUM(i);
			if (check->v.takedamage != DAMAGE_AIM || check == ent || (teamplay.value && ent->v.team > 0 && ent->v.team == check->v.team))
				continue;

			vec3_t checkPos;
			for (int j = 0; j < 3; j++)
				checkPos[j] = check->v.origin[j] + 0.5 * (check->v.mins[j] + check->v.maxs[j]);

			VectorSubtract(checkPos, start, dir);
			VectorNormalize(dir);
			dist = DotProduct(dir, pr_global_struct->v_forward);

			if (dist < bestdist)
				continue; // too far to turn

			tr = SV_Move(start, vec3_origin, vec3_origin, checkPos, false, ent);
			if (tr.ent == check) {
				// can shoot at this one
				bestdist = dist;
				bestent = check;
			}
		}

		if (bestent) {
			VectorSubtract(bestent->v.origin, ent->v.origin, dir);
			dist = DotProduct(dir, pr_global_struct->v_forward);
			VectorScale(pr_global_struct->v_forward, dist, end);
			end[2] = dir[2];
			VectorNormalize(end);
			return end;
		}
		else {
			return bestdir;
		}
	}

	void changeyaw(void)
	{
		edict_t* ent;
		float		ideal, current, move, speed;

		ent = PROG_TO_EDICT(pr_global_struct->self);
		current = anglemod(ent->v.angles[1]);
		ideal = ent->v.ideal_yaw;
		speed = ent->v.yaw_speed;

		if (current == ideal)
			return;
		move = ideal - current;
		if (ideal > current)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}

		ent->v.angles[1] = anglemod(current + move);
	}

	sizebuf_t* WriteDest(float dest)
	{
		int entnum;
		edict_t* ent;


		#define	MSG_BROADCAST	0		// unreliable to all
		#define	MSG_ONE			1		// reliable to one (msg_entity)
		#define	MSG_ALL			2		// reliable to all
		#define	MSG_INIT		3		// write to the init string



		switch (static_cast<int>(dest))
		{
		case MSG_BROADCAST:
			return &sv.datagram;

		case MSG_ONE:
			ent = PROG_TO_EDICT(pr_global_struct->msg_entity);
			entnum = NUM_FOR_EDICT(ent);
			if (entnum < 1 || entnum > svs.maxclients)
				PR_RunError("WriteDest: not a client");
			return &svs.clients[entnum - 1].message;

		case MSG_ALL:
			return &sv.reliable_datagram;

		case MSG_INIT:
			return &sv.signon;

		default:
			PR_RunError("WriteDest: bad destination");
			break;
		}

		return nullptr;
	}

	void WriteByte(float dest, float value)
	{
		MSG_WriteByte(WriteDest(dest), static_cast<int>(value));
	}

	void WriteChar(float dest, float value)
	{
		MSG_WriteChar(WriteDest(dest), static_cast<int>(value));
	}

	void WriteShort(float dest, float value)
	{
		MSG_WriteShort(WriteDest(dest), static_cast<int>(value));
	}

	void WriteLong(float dest, float value)
	{
		MSG_WriteLong(WriteDest(dest), static_cast<int>(value));
	}

	void WriteAngle(float dest, float value)
	{
		MSG_WriteAngle(WriteDest(dest), value);
	}

	void WriteCoord(float dest, float value)
	{
		MSG_WriteCoord(WriteDest(dest), value);
	}

	void WriteString(float dest, const char* s0)
	{
		MSG_WriteString(WriteDest(dest), (char *)s0);
	}

	void WriteEntity(float dest, edict_t* e0)
	{
		MSG_WriteShort(WriteDest(dest), NUM_FOR_EDICT(e0));
	}

	void makestatic(edict_t* ent)
	{
		MSG_WriteByte(&sv.signon, svc_spawnstatic);

		MSG_WriteByte(&sv.signon, SV_ModelIndex(pr_strings + ent->v.model));

		MSG_WriteByte(&sv.signon, ent->v.frame);
		MSG_WriteByte(&sv.signon, ent->v.colormap);
		MSG_WriteByte(&sv.signon, ent->v.skin);
		for (int i = 0; i < 3; i++)
		{
			MSG_WriteCoord(&sv.signon, ent->v.origin[i]);
			MSG_WriteAngle(&sv.signon, ent->v.angles[i]);
		}

		// throw the entity away now
		ED_Free(ent);
	}

	void setspawnparms(edict_t* ent)
	{
		int i = NUM_FOR_EDICT(ent);
		if (i < 1 || i > svs.maxclients)
			PR_RunError("Entity is not a client");

		// copy spawn parms out of the client_t
		client_t* client = &svs.clients[i - 1];

		for (i = 0; i < NUM_SPAWN_PARMS; i++)
			(&pr_global_struct->parm1)[i] = client->spawn_parms[i];
	}

	void changelevel(const char* levelName)
	{
		if (svs.changelevel_issued)
			return;

		svs.changelevel_issued = true;

		Cbuf_AddText(va("changelevel %s\n", levelName));
	}
}