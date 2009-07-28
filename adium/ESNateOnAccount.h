//
//  ESNateOnAccount.h
//
// code by mario ( xxfree86@gmail.com )

#import <Cocoa/Cocoa.h>
#import <AdiumLibpurple/CBpurpleAccount.h>
#import "nateon.h"

@interface ESNateOnAccount : CBPurpleAccount <AIAccount_Files>{

}
- (void)toggleNameAction:(NSMenuItem *)sender;
- (void)viewBuddiesBy: (int) choice;
- (void)setUserAlias:(NateonUser *)user Choice:(int)choice;
 @end
