#include "interface.h"
#include "engine/iserverplugin.h"
#include "eiface.h"

#include "icommandline.h"

#include <sourcehook/sourcehook_impl.h>
#include <sourcehook/sourcehook.h>
 
SourceHook::Impl::CSourceHookImpl g_SourceHook;

extern SourceHook::ISourceHook *g_SHPtr;
extern int g_PLID;

SourceHook::ISourceHook *g_SHPtr = &g_SourceHook;
int g_PLID = 0xC5606464;

IServerGameClients	*gameclients = NULL;

SH_DECL_HOOK3_void( IServerGameClients, GetPlayerLimits, const, 0, int &, int &, int & );

int g_iMaxPlayers;

class PLR: public IServerPluginCallbacks
{
public:
	PLR();

	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void ) {}
	virtual void			UnPause( void ) {}
	virtual const char     *GetPluginDescription( void );
	virtual void			LevelInit( char const *pMapName ) {}
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax ) {}
	virtual void			GameFrame( bool simulating ) {}
	virtual void			LevelShutdown( void ) {}
	virtual void			ClientActive( edict_t *pEntity ) {}
	virtual void			ClientFullyConnect( edict_t *pEntity ) {}
	virtual void			ClientDisconnect( edict_t *pEntity ) {}
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername ) {}
	virtual void			SetCommandClient( int index ) { m_iClientCommandIndex = index; }
	virtual void			ClientSettingsChanged( edict_t *pEdict ) { }
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen ) { return PLUGIN_CONTINUE; }
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args ) { return PLUGIN_CONTINUE; }
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID ) { return PLUGIN_CONTINUE; }
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue ) {}
	virtual void			OnEdictAllocated( edict_t *edict ) {}
	virtual void			OnEdictFreed( const edict_t *edict  ) {}	

	virtual int GetCommandIndex() { return m_iClientCommandIndex; }

	void					Hook_GetPlayerLimits( int &minplayer, int &maxplayers, int &defaultmaxplayers );
private:
	int m_iClientCommandIndex;
};


PLR g_PLR;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(PLR, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_PLR );

PLR::PLR() :
	m_iClientCommandIndex( 0 )
{
}

bool PLR::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	gameclients = (IServerGameClients*)gameServerFactory(INTERFACEVERSION_SERVERGAMECLIENTS, NULL);
	if( !gameclients )
	{
		return false;
	}

	const char *pszCmdLineMax;
	if( !CommandLine()->CheckParm("-maxplayers", &pszCmdLineMax )
		&& !CommandLine()->CheckParm("+maxplayers", &pszCmdLineMax ) )
	{
		return false;
	}

	g_iMaxPlayers = clamp( atoi( pszCmdLineMax ), 1, ABSOLUTE_PLAYER_LIMIT );

	SH_ADD_HOOK( IServerGameClients, GetPlayerLimits, gameclients, SH_MEMBER(this, &PLR::Hook_GetPlayerLimits), false );

	return true;
}

void PLR::Unload( void )
{
	SH_REMOVE_HOOK( IServerGameClients, GetPlayerLimits, gameclients, SH_MEMBER(this, &PLR::Hook_GetPlayerLimits), false );
}

void PLR::Hook_GetPlayerLimits( int &minplayers, int &maxplayers, int &defaultmaxplayers )
{
	minplayers = 1;
	maxplayers = ABSOLUTE_PLAYER_LIMIT;
	defaultmaxplayers = g_iMaxPlayers;
	
	RETURN_META( MRES_SUPERCEDE );
}

const char *PLR::GetPluginDescription( void )
{
	return "TF2/HL2DM Player Limit Remover";
}

