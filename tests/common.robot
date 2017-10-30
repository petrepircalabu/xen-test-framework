*** Settings ***
Library		OperatingSystem
Library		XTFLibrary

*** Keywords ***
Run DomU
	[Arguments]			${cfg}
	File Should Exist		${cfg}
	${domid}=			XTFLibrary.Create	${cfg}
	XTFLibrary.Resume		${domid}
	XTFLibrary.WaitForCompletion	${domid}
	XTFLibrary.Cleanup		${domid}		10
