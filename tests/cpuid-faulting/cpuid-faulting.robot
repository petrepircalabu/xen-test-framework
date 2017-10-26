*** Settings ***
Resource	../common.robot

*** Test Cases ***
test-hvm32-cpuid-faulting
	[Tags]		hvm32
	Run DomU	${CURDIR}/test-hvm32-cpuid-faulting.cfg

test-hvm32pae-cpuid-faulting
	[Tags]		hvm32pae
	Run DomU	${CURDIR}/test-hvm32pae-cpuid-faulting.cfg

test-hvm32pse-cpuid-faulting
	[Tags]		hvm32pse
	Run DomU	${CURDIR}/test-hvm32pse-cpuid-faulting.cfg

test-hvm64-cpuid-faulting
	[Tags]		hvm64
	Run DomU	${CURDIR}/test-hvm64-cpuid-faulting.cfg

test-pc32pae-cpuid-faulting
	[Tags]		pv32pae
	Run DomU	${CURDIR}/test-pv32pae-cpuid-faulting.cfg

test-pv64-cpuid-faulting
	[Tags]		pv64
	Run DomU	${CURDIR}/test-pv64-cpuid-faulting.cfg

