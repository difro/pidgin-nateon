//
//  ESNateOnLibpurpleServicePlugin.m
//
// code by mario ( xxfree86@gmail.com )

#import "ESNateOnLibpurpleServicePlugin.h"
#import "ESNateOnService.h"

@implementation ESNateOnLibpurpleServicePlugin

extern BOOL purple_init_nateon_plugin(void);

- (void)installPlugin
{
	NateOnService = [[[ESNateOnService alloc] init] retain];
	purple_init_nateon_plugin();
}

- (void)installLibpurplePlugin
{
}

- (void)loadLibpurplePlugin
{
	//No action needed
}

- (void)dealloc
{
	[NateOnService release];

	[super dealloc];
}

- (void)uninstallPlugin
{

}

- (NSString *)libpurplePluginPath
{
	return [[NSBundle bundleForClass:[self class]] resourcePath];
}

@end
