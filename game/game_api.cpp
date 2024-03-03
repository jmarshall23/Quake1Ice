// game_api.cpp
//

#include "../quakedef.h"
#include "game_local.h"

namespace QuakeAPI
{
	void SetMinMaxSize(edict_t* e, float* min, float* max, qboolean rotate)
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

	//void setsize(edict_t* e, const vec3_t & min, const vec3_t & max)
	//{
//		SetMinMaxSize(e, min, max, false);
	//}

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


}