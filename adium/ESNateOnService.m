//
//  ESNateOnService.m
//
// code by mario ( xxfree86@gmail.com )

#import "ESNateOnService.h"
#import <AIUtilities/AITigerCompatibility.h> 
#import "ESNateOnAccount.h"
#import <Adium/AIAccountViewController.h>
#import <Adium/AIStatusControllerProtocol.h>
#import <AIUtilities/AIImageAdditions.h>
#import <AIUtilities/AIStringUtilities.h>
#import <Adium/AISharedAdium.h> 

@implementation ESNateOnService
//Account Creation
- (Class)accountClass{
	return [ESNateOnAccount class];
}

//Service Description
- (NSString *)serviceCodeUniqueID{
	return @"libpurple-NateOn";
}
- (NSString *)serviceID{
	return @"NateOn";
}
- (NSString *)serviceClass{
	return @"NateOn";
}
- (NSString *)shortDescription{
	return @"NateOn";
}
- (NSString *)longDescription{
	return @"NateOn IM";
}
- (NSCharacterSet *)allowedCharacters{
	return [NSCharacterSet characterSetWithCharactersInString:@"+abcdefghijklmnopqrstuvwxyz0123456789@._- "];
}
- (NSCharacterSet *)ignoredCharacters{
	return [NSCharacterSet characterSetWithCharactersInString:@""];
}
- (BOOL)caseSensitive{
	return NO;
}
- (BOOL)canCreateGroupChats{
	return NO;
}
//Passwords are optional - XXX right now this means we totally ignore the password field
- (BOOL)requiresPassword
{
	return YES;
}
- (AIServiceImportance)serviceImportance{
	return AIServiceSecondary;
}
/*!
* @brief Placeholder string for the UID field
 */
- (NSString *)UIDPlaceholder
{
	return AILocalizedString(@"username@nate.com", "NateOn accounts");
}

- (NSImage *)defaultServiceIconOfType:(AIServiceIconType)iconType
{
	NSImage *baseImage = [NSImage imageNamed:@"NateOn" forClass:[self class]];

	if (iconType == AIServiceIconSmall) {
		baseImage = [baseImage imageByScalingToSize:NSMakeSize(16, 16)];
	}

	return baseImage;
}

- (void)registerStatuses{
	
	[[adium statusController] registerStatus:STATUS_NAME_AVAILABLE
							 withDescription:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_AVAILABLE]
									  ofType:AIAvailableStatusType
								  forService:self];
	
	[[adium statusController] registerStatus:STATUS_NAME_AWAY
							 withDescription:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_AWAY]
									  ofType:AIAwayStatusType
								  forService:self];
/*	
	[[adium statusController] registerStatus:STATUS_NAME_BRB
							 withDescription:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_BRB]
									  ofType:AIAwayStatusType
								  forService:self];
*/	
	[[adium statusController] registerStatus:STATUS_NAME_BUSY
							 withDescription:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_BUSY]
									  ofType:AIAwayStatusType
								  forService:self];
	
	[[adium statusController] registerStatus:STATUS_NAME_PHONE
							 withDescription:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_PHONE]
									  ofType:AIAwayStatusType
								  forService:self];
/*	
	[[adium statusController] registerStatus:STATUS_NAME_LUNCH
							 withDescription:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_LUNCH]
									  ofType:AIAwayStatusType
								  forService:self];
*/
	[[adium statusController] registerStatus:STATUS_NAME_INVISIBLE
							 withDescription:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_INVISIBLE]
									  ofType:AIInvisibleStatusType
								  forService:self];

	
}
@end
