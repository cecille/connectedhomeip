from rich.console import Console
from rich import inspect, pretty
from chip.ChipStack import *
import coloredlogs
import chip.native
import chip.logging
import chip.FabricAdmin
import chip.discovery
import chip.CertificateAuthority
import logging
import builtins
import chip.dynamic_server.Server as Server
import argparse
import atexit

_fabricAdmins = None


def ReplInit(debug):
    #
    # Install the pretty printer that rich provides to replace the existing
    # printer.
    #
    pretty.install(indent_guides=True, expand_all=True)

    console = Console()

    console.rule('Matter REPL')
    console.print('''
            [bold blue]

            Welcome to the Matter Python REPL!

            For help, please type [/][bold green]matterhelp()[/][bold blue]

            To get more information on a particular object/class, you can pass
            that into [bold green]matterhelp()[/][bold blue] as well.

            ''')
    console.rule()

    coloredlogs.install(level='DEBUG')
    chip.logging.RedirectToPythonLogging()

    if debug:
        logging.getLogger().setLevel(logging.DEBUG)
    else:
        logging.getLogger().setLevel(logging.WARN)


certificateAuthorityManager = None


def StackShutdown():
    certificateAuthorityManager.Shutdown()
    builtins.chipStack.Shutdown()


def matterhelp(classOrObj=None):
    if (classOrObj is None):
        inspect(builtins.devCtrl, methods=True, help=True, private=False)
        inspect(mattersetlog)
        inspect(mattersetdebug)
    else:
        inspect(classOrObj, methods=True, help=True, private=False)


def mattersetlog(level):
    logging.getLogger().setLevel(level)


def mattersetdebug(enableDebugMode: bool = True):
    ''' Enables debug mode that is utilized by some Matter modules
        to better facilitate debugging of failures (e.g throwing exceptions instead
        of returning well-formatted results).
    '''
    builtins.enableDebugMode = enableDebugMode


console = Console()

parser = argparse.ArgumentParser()
parser.add_argument(
    "-p", "--storagepath", help="Path to persistent storage configuration file (default: /tmp/repl-storage.json)", action="store", default=None)
parser.add_argument(
    "-d", "--debug", help="Set default logging level to debug.", action="store_true")
parser.add_argument(
    "-m", "--mode", help="Whether to start the REPL in controller or server modes.", action="store", default="controller")

args = parser.parse_args()

chip.native.Init()

ReplInit(args.debug)

if (args.mode == "controller"):
    if args.storagepath is None:
        args.storagepath = '/tmp/repl-controller.json'

    console.print(
        '\n\n[bold red]>>>> Starting REPL in Controller Mode <<<<\n\n')

    chipStack = ChipStack(persistentStoragePath=args.storagepath, stackInitType=chip.ChipStack.StackInitType.Controller)
    certificateAuthorityManager = chip.CertificateAuthority.CertificateAuthorityManager(chipStack, chipStack.GetStorageManager())

    certificateAuthorityManager.LoadAuthoritiesFromStorage()

    if (len(certificateAuthorityManager.activeCaList) == 0):
        ca = certificateAuthorityManager.NewCertificateAuthority()
        ca.NewFabricAdmin(vendorId=0xFFF1, fabricId=1)
    elif (len(certificateAuthorityManager.activeCaList[0].adminList) == 0):
        certificateAuthorityManager.activeCaList[0].NewFabricAdmin(vendorId=0xFFF1, fabricId=1)

    caList = certificateAuthorityManager.activeCaList

    devCtrl = caList[0].adminList[0].NewController()
    builtins.devCtrl = devCtrl

    console.print(
        '\n\n[blue]Default CHIP Device Controller has been initialized to manage [bold red]fabricAdmins[0][blue], and is available as [bold red]devCtrl')
else:
    if args.storagepath is None:
        args.storagepath = '/tmp/repl-server.json'

    console.print(
        '\n\n[bold red]>>>> Starting REPL in Server Mode <<<<\n\n')

    chipStack = ChipStack(persistentStoragePath=args.storagepath, stackInitType=chip.ChipStack.StackInitType.Server)
    server = Server.Server()

    console.print(
        '\n\n[blue]Default CHIP Server instance has been initialized and is available as [bold red]server')

atexit.register(StackShutdown)
