//
//  ESNateOnLibpurpleServicePlugin.h
//
// code by mario ( xxfree86@gmail.com )

#import <Cocoa/Cocoa.h>
#import <Adium/AIPlugin.h>
#import <AdiumLibpurple/AILibpurplePlugin.h>

@class ESNateOnService;

@interface ESNateOnLibpurpleServicePlugin : AIPlugin <AILibpurplePlugin> {
	ESNateOnService *NateOnService;
}

@end
