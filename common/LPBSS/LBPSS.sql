USE [master]
--设置最大内存 
EXEC sys.sp_configure N'show advanced options', N'1'  RECONFIGURE WITH OVERRIDE
EXEC sys.sp_configure N'max server memory (MB)', N'2048'
RECONFIGURE WITH OVERRIDE
EXEC sys.sp_configure N'show advanced options', N'0'  RECONFIGURE WITH OVERRIDE

USE LPBSS
GO

--删除已经存在的表、存储过程、视图、触发器
select identity(int,1,1) flag,[name] names,xtype into #tmp
from sysobjects where xtype in('V','U','P','TF','FN')
--执行上一步之后再执行下面：
--第二步循环删除
declare @tb varchar(1000),@a int,@b int,@sql varchar(8000),@xtype varchar(50)
select @a=min(flag),@b=max(flag) from #tmp
while @a<=@b
 begin
   select @tb=names, @xtype = xtype from #tmp where flag=@a
   if(@xtype='V')
   begin
      set @sql = 'drop view '+ @tb;
   end 
   if(@xtype='P')
   begin
      set @sql = 'drop procedure '+ @tb;
   end 
   if(@xtype='TF'or @xtype='FN')
   begin
      set @sql = 'drop function  '+ @tb;
   end 
   if(@xtype='U')
   begin
      set @sql = 'drop Table ['+ @tb+']';
   end 
   exec(@sql)
   set @a=@a+1
end
drop table #tmp

---------------------------------------------------Table Begin--------------------------------------------------------



/*==============================================================*/
/* Table: [OperateLogType]*/
/*==============================================================*/

CREATE TABLE [dbo].[OperateLogType](
	[Id] [int] IDENTITY(1,1) NOT NULL,
	[TypeName] [nvarchar](50) NOT NULL,
	[TypeValue] [int] NOT NULL
) ON [PRIMARY]

GO


/*==============================================================*/
/* Table: CardException                                         */
/*==============================================================*/
create table dbo.CardException (
   id                   int                  identity(1, 1),
   CardNum              nvarchar(50)         not null,
   CardType				      int					         not null,
   WorkSiteId           int                  NOT NULL default (0),
   OccTime              datetime             not null,
   OkTime               datetime             null,
   Msg                  nvarchar(256)        null,
   IsRead               int                  not null default (0),
   constraint PK_CardException primary key (id)
         on "PRIMARY"
)
on "PRIMARY"
go

/*==============================================================*/
/* Table: CardInformation                                       */
/*==============================================================*/
create table dbo.CardInformation (
   CardNum              nvarchar(50)         not null,
   CardType		int		     not null,
   Version              nvarchar(1024)         null,
   Power                nvarchar(50)         null,
   OccTime              datetime             null,
   InDBTime             datetime             not null  default getdate(),
   Status				int					 null
   constraint PK_CardInformation primary key (CardNum,CardType)
         on "PRIMARY"
)
on "PRIMARY"
go

/*==============================================================*/
/* Table: CardLoc                                               */
/*==============================================================*/
create table dbo.CardLoc (
   CardNum              nvarchar(50)         collate Chinese_PRC_CI_AS not null,
   CardType				int					 not null,
   WorkSiteId           int                  not null,
   x                    float                not null,
   y                    float                not null,
   z                    float                not null constraint DF_CardLoc_z default (0),
   speed                float                    null,
   OccTime              datetime             not null,
   Distance             float                not null constraint DF_CardLoc_Distance default (0)
   constraint PK_CardLoc primary key (CardNum,CardType)
         on "PRIMARY"
)
on "PRIMARY"
go
/*==============================================================*/
/* Table: CardLoc_2D 2D专用                                     */
/*==============================================================*/
CREATE TABLE [dbo].[CardLoc_2D](
	[CardNum] [nvarchar](50) NOT NULL,
	[CardType] [int] NOT NULL,
	[WorkSiteId] [int] NOT NULL,
	[x] [float] NOT NULL,
	[y] [float] NOT NULL,
	[speed] [float] NULL,
	[OccTime] [datetime] NOT NULL,
    [Distance] [float]   NOT NULL constraint DF_CardLoc_2D_Distance default (0)
 CONSTRAINT [PK_CardLoc_2D] PRIMARY KEY CLUSTERED 
(
	[CardNum] ASC,
	[CardType] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]
GO

/*==============================================================*/
/* Table: CardLowPower                                          */
/*==============================================================*/
create table dbo.CardLowPower (
   id                   int                  identity(1, 1),
   CardNum              nvarchar(50)         not null,
   CardType				      int					         not null,
   Power                nvarchar(50)         null,
   WorkSiteId           int                  not null default(0),
   OccTime              datetime             not null,
   InDBTime             datetime             not null default (getdate()),
   IsRead               int                  not null default (0),
   constraint PK_CardLowPower primary key (id)
         on "PRIMARY"
)
on "PRIMARY"
go

/*==============================================================*/
/* Table: CardSos                                               */
/*==============================================================*/
create table dbo.CardSos (
   id                   int                  identity(1, 1),
   CardNum              nvarchar(50)         collate Chinese_PRC_CI_AS not null,
   CardType				int					 not null,
   WorkSiteId           int                  not null constraint DF_CardSos_WorkSiteId default (0),
   OccTime              datetime             not null,
   LastTime							datetime						 not null,
   IsRead               int                  not null constraint DF_CardSos_IsRead default (0),
   constraint PK_CardSos primary key (id)
         on "PRIMARY"
)
on "PRIMARY"
go

/*==============================================================*/
/* Table: LinkHistory                                           */
/*==============================================================*/
create table dbo.LinkHistory (
   id                   int                  identity(1, 1),
   WorkSiteId           int                  not null,
   Port                 int                  not null,
   ConnDevNum           int                  not null constraint DF_LinkHistory_ConnDevNum default (0),
   OccTime              datetime             not null,
   InDBTime             datetime             not null constraint DF_LinkHistory_InDBTime default getdate(),
   constraint PK_LinkHistory primary key (id)
         on "PRIMARY"
)
on "PRIMARY"
go

/*==============================================================*/
/* Table: LinkInfo                                              */
/*==============================================================*/
CREATE TABLE [dbo].[LinkInfo](
	[WorkSiteId] [int] NOT NULL,
	[Port] [int] NOT NULL,
	[ConnDevNum] [int] NOT NULL CONSTRAINT [DF_LinkInfo_ConnDevNum]  DEFAULT ((0)),
	[OccTime] [datetime] NOT NULL,
 CONSTRAINT [PK_LinkInfo] PRIMARY KEY CLUSTERED 
(
	[ConnDevNum] ASC
)WITH (IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]
go

/*==============================================================*/
/* Table: RssiFix                                               */
/*==============================================================*/
create table dbo.RssiFix (
   WorkSiteId           int                  not null,
   RssiFixValue         int                  not null constraint DF_RssiFix_RssiFixValue default (0),
   constraint PK_RssiFix primary key (WorkSiteId)
         on "PRIMARY"
)
on "PRIMARY"
go

/*==============================================================*/
/* Table: RssiWeight                                            */
/*==============================================================*/
CREATE TABLE [dbo].[RssiWeight](
	[rssi] [int] NOT NULL,
	[weight] [int] NULL,
 CONSTRAINT [PK_RssiWeight] PRIMARY KEY CLUSTERED 
(
	[rssi] ASC
)WITH (IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]
Go

/*==============================================================*/
/* Table: WorkSite 2D新增x2,y2、mapid字段              */
/*==============================================================*/
create table dbo.WorkSite (
   WorkSiteId           int                  not null,
   Address              nvarchar(100)        collate Chinese_PRC_CI_AS null,
   Type                 int                  null,
   Dimension            int                  null,
   x                    float                null,
   y                    float                null,
   z                    float                not null constraint DF_WorkSite_z default (0),
   x2                   float                null,
   y2                   float                null,
   rotaryW		float		     not null constraint DF_WorkSite_rotaryW default (1),
   rotaryX		float		     not null constraint DF_WorkSite_rotaryX default (0),
   rotaryY		float		     not null constraint DF_WorkSite_rotaryY default (0),
   rotaryZ		float		     not null constraint DF_WorkSite_rotaryZ default (0),
   VisualOffsetX	float		     not null constraint DF_WorkSite_VisualOffsetX default (0),
   VisualOffsetY	float		     not null constraint DF_WorkSite_VisualOffsetY default (0),
   VisualOffsetZ	float		     not null constraint DF_WorkSite_VisualOffsetZ default (0),
   mapid            int              null,
   OccTime          datetime         null,
   InDBTime         datetime         null,
   Team             int              null,
   Version			varchar(100)	 null constraint PK_WorkSite primary key (WorkSiteId) on "PRIMARY"
)
on "PRIMARY"
go

/*==============================================================*/
/* Table: WorkSiteAlarm                                         */
/*==============================================================*/
create table dbo.WorkSiteAlarm (
   id                   int                  identity(1, 1),
   WorkSiteId           int                  not null,
   OccTime              datetime             not null,
   OkTime               datetime             null,
   Msg                  nvarchar(256)        collate Chinese_PRC_CI_AS null,
   IsRead               int                  not null constraint DF_WorkSiteMsg_IsRead default (0),
   constraint PK_WorkSiteMsg primary key (id)
         on "PRIMARY"
)
on "PRIMARY"
go

/*==============================================================*/
/* Table: WorkSitePass   2D新增x2,y2                            */
/*==============================================================*/
create table dbo.WorkSitePass (
   id                   int                  identity(1, 1),
   WorkSiteId           int                  not null,
   STime                datetime             not null,
   ETime                datetime             null,
   CardNum              nvarchar(50)         collate Chinese_PRC_CI_AS not null,
   CardType				int					 not null,
	x					float				 NOT NULL,
	y					float				 NOT NULL,
	z					float				 NOT NULL,
	x2                  float                NULL,
	y2                  float                NULL
   constraint PK_WorkSitePass primary key nonclustered (id)
         on "PRIMARY"
)
Go

/*==============================================================*/
/* Table:  CardLoc_Interval                                     */
/*==============================================================*/
CREATE TABLE [dbo].[CardLoc_Interval](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[CardNum] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NOT NULL,
	[CardType] int	NOT NULL,
	[WorkSiteId] [int] NOT NULL,
	[x] [float] NOT NULL,
	[y] [float] NOT NULL,
	[z] [float] NOT NULL CONSTRAINT [DF_CardLoc_Interval_z]  DEFAULT ((0)),
	[speed] [float] NULL,
	[OccTime] [datetime] NOT NULL,
 CONSTRAINT [PK_CardLoc_Interval] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]
Go

/*==============================================================*/
/* Table: CardLoc_Interval_2D  2D专用                           */
/*==============================================================*/
CREATE TABLE [dbo].[CardLoc_Interval_2D](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[CardNum] [nvarchar](50) NOT NULL,
	[CardType] [int] NOT NULL,
	[WorkSiteId] [int] NOT NULL,
	[x] [float] NOT NULL,
	[y] [float] NOT NULL,
	[speed] [float] NULL,
	[OccTime] [datetime] NOT NULL,
	[IsOnWorksite] [int] NOT NULL,
	[Objectid] [int] NOT NULL,
 CONSTRAINT [PK_CardLoc_Interval_2D] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]
GO

/*==============================================================*/
/* Index: CT_WorkSitePass_STime                                 */
/*==============================================================*/
create clustered index CT_WorkSitePass_STime on dbo.WorkSitePass (
STime ASC
)
go

/*==============================================================*/
/* Table: Settings                                              */
/*==============================================================*/
create table dbo.Settings (
   Item                 varchar(50)          collate Chinese_PRC_CI_AS not null,
   Value            varchar(50)          collate Chinese_PRC_CI_AS not null,
   Description          varchar(100)         collate Chinese_PRC_CI_AS null,
   constraint PK_Settings primary key (Item)
         on "PRIMARY"
)
on "PRIMARY"
go

/*==============================================================*/
/* Table: AreaMode		                                        */
/*==============================================================*/
create table dbo.areaMode
(
    ModeID  int primary key not null identity(1,1),
    ModeName nvarchar(50)   not null, 
)
go

/*==============================================================*/
/* Table: AreaType		                                        */
/*==============================================================*/
create table dbo.areaType
(
    id	     int primary key not null identity(1,1),
    Name     nvarchar(50)   not null, 
)
go

/*==============================================================*/
/* Table: Area			                                        */
/*==============================================================*/
Create table dbo.Area
(
	id		int 				primary key not null identity(1,1),
	number		varchar(20)			NULL, 
	aName		nvarchar(50)	Not NULL,
	Mode		int				NULL default 0,
	Type	 	int				NULL default 0,
	parentId	int				NULL default 0,
	isAllow		int				Not Null default 1,
	outSpanTime int				NOT NULL default -1,
	personSize	int				NULL default 0,
	RssiThs   int               NULL ,
        x               float                           null,
        y               float                           null,
	LayerName	nvarchar(100)	NULL			
)

GO


/*==============================================================*/
/* Table: AlarmType			                                    */
/*==============================================================*/

CREATE TABLE [dbo].[AlarmType](
	[AlarmID] [int] NOT NULL,
	[AlarmName] [nvarchar](50) NOT NULL,
 CONSTRAINT [PK_AlarmType] PRIMARY KEY CLUSTERED 
(
	[AlarmID] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

/*==============================================================*/
/* Table: areaAlarm			                                    */
/*==============================================================*/
CREATE TABLE [dbo].[areaAlarm](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[cardNum] [varchar](50) COLLATE Chinese_PRC_CI_AS,
	[CardType]			int		 not null,	
	[workSiteNum] [varchar](50) COLLATE Chinese_PRC_CI_AS NULL,
	[workAreaId] [int] NULL,
	[beginTime] [datetime] NULL,
	[endTime] [datetime] NULL,
	[isRead] [int] NULL,
 CONSTRAINT [PK_areaAlarm] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]

GO

/*==============================================================*/
/* Table: areaOverTimeAlarm  			                        */
/*==============================================================*/
create table areaOverTimeAlarm
(
	id          	int   primary key not null identity(1,1),
	cardNum			varchar(50)		  NOT NULL,
	cardType		int				  NOT NULL,
	worksiteNum		int				  NULL,
	areaId			int				  NULL,
	inTime			datetime		  NULL,				 
	beginTime		datetime		  NOT NULL,
	endTime			dateTime		  NULL,
	readTime		dateTime		  NULL,
	isRead			int				  NULL
)
Go

/*==============================================================*/
/* Table: areaPurview			                        */
/*==============================================================*/
create table dbo.areaPurview
(
	id   		int		primary key 	identity(1,1)   not null,
	areaId		int		not 	null,
	objectid	int 	not 	null,
)
go

/*==============================================================*/
/* Table: corAreaWorkiste		                                */
/*==============================================================*/
create table dbo.corAreaWorksite
(
	id          	int   primary key not null identity(1,1),
	WorksiteId  	int  not null,
	areaId			int
)
go

/*==============================================================*/
/* Table: department		                                    */
/*==============================================================*/
CREATE TABLE [dbo].[department](
	[id] [int]  identity(1,1) NOT NULL,
	[number] [varchar](50) COLLATE Chinese_PRC_CI_AS NULL,
	[name] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NOT NULL,
	[parentId] [int] NULL,
	[leader] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NULL,
	[telephone] [varchar](20) COLLATE Chinese_PRC_CI_AS NULL,
PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]

GO


/*==============================================================*/
/* Table: classTeam		                                        */
/*==============================================================*/
CREATE TABLE [dbo].[classTeam](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[number] [varchar](50) COLLATE Chinese_PRC_CI_AS NULL,
	[name] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NOT NULL,
	[departId] [int] NULL,
	[leader] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NULL,
	[telephone] [varchar](50) COLLATE Chinese_PRC_CI_AS NULL,
PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO

/*==============================================================*/
/* Table: officePosition		                                */
/*==============================================================*/
CREATE TABLE [dbo].[officePosition](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[name] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NOT NULL,
	[position] [float]  NULL,
	[number] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NULL,
	[level] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NULL,
	[isCadres] [bit] NULL CONSTRAINT [DF_officePosition_isCadres]  DEFAULT ((0)),
	[iconPath] [nvarchar](200) NULL,
	[isNurse] [int] NOT NULL DEFAULT ((1)),
 CONSTRAINT [PK_officePosition] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, IGNORE_DUP_KEY = OFF) ON [PRIMARY],
 CONSTRAINT [IX_officePosition] UNIQUE NONCLUSTERED 
(
	[name] ASC
)WITH (PAD_INDEX  = OFF, IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]
go


/*==============================================================*/
/* Table: Person		                                       */
/*==============================================================*/
--逐步过渡
CREATE TABLE [dbo].[Person](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[CardNumber] [nvarchar](50) NOT NULL,
	[CardType] [int] NOT NULL DEFAULT ((3)),
	[PersonName] [nvarchar](50) NOT NULL,
	[PersonNo] [nvarchar](50) NULL,
	[DepartmentId] [int] NOT  NULL DEFAULT ((0)),
	[ClassTeamId] [int] NULL,
	[OfficePositionId] [int] NULL,
	[Sex] [nvarchar](7) NULL,
	[Mobile] [nvarchar](50) NULL,
	[Telephone] [nvarchar](50) NULL,
	[IdentifyNum] [nvarchar](50) NULL,
	[PhotoExtends] [nvarchar](50) NULL,
	[Photo] [image] NULL,
	[Birthday] [datetime] NULL,
	[InTime] [datetime] NULL,
	[OutTime] [datetime] NULL,
 CONSTRAINT [PK__person__3213E83F60A75C0F] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]

GO

/****** Object:  Table [dbo].[Employee]    Script Date: 03/24/2015 14:05:44 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Employee](
	[PersonId] [int] NOT NULL,
	[Number] [nvarchar](50) NOT NULL,
	[DepartmentId] [int] NOT NULL,
	[ClassTeamId] [int] NULL,
	[OfficePositionId] [int] NOT NULL,
 CONSTRAINT [PK_Employee] PRIMARY KEY CLUSTERED 
(
	[PersonId] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

/****** Object:  Table [dbo].[Customer]    Script Date: 03/24/2015 14:07:23 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Customer](
	[PersonId] [int] NOT NULL,
	[NurseUnitId] [int] NOT NULL,
 CONSTRAINT [PK_Customer] PRIMARY KEY CLUSTERED 
(
	[PersonId] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

/****** Object:  Table [dbo].[NurseUnit]    Script Date: 03/24/2015 14:08:16 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[NurseUnit](
	[id] [int] IDENTITY(1,1) NOT NULL PRIMARY KEY,
	[AreaId] [int] NOT NULL,
	[Address] [nvarchar](50) NOT NULL UNIQUE,
	[MattressNum] [int] NULL,
	[PagerNum] [int] NULL
) ON [PRIMARY]

GO

/****** Object:  Table [dbo].[NurseRelation]    Script Date: 03/24/2015 14:10:20 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[NurseRelation](
	[CustomerId] [int] NOT NULL,
	[NurseId] [int] NOT NULL,
 CONSTRAINT [PK_NurseRelation] PRIMARY KEY CLUSTERED 
(
	[CustomerId] ASC,
	[NurseId] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

/****** Object:  Table [dbo].[PagerCallAlarm]    Script Date: 03/24/2015 14:23:17 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[PagerCallAlarm](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[PagerNum] [int] NOT NULL,
	[OccTime] [datetime] NOT NULL,
	[LastTime] [datetime] NOT NULL,
	[IsRead] [int] NOT NULL DEFAULT ((0)),
 CONSTRAINT [PK_PagerAlarm] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

/****** Object:  proc [dbo].[SP_InsertPagerCallAlarm]    Script Date: 03/30/2015 14:40:17 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE SP_InsertPagerCallAlarm
(
@num INT,
@occtime DATETIME
)
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
	
	DECLARE @id INT,
			@pagerNum INT,
			@lastTime DATETIME
			
    SET @id = NULL;
    SELECT TOP 1 @id=id, @pagerNum=PagerNum, @lastTime=LastTime FROM PagerCallAlarm WHERE PagerNum=@num ORDER BY id DESC;
    
    IF @id IS NULL OR DATEDIFF(SECOND, @lastTime, @occtime) >= 30
		INSERT INTO PagerCallAlarm(PagerNum, OccTime, LastTime) VALUES(@num, @occtime, @occtime);
	ELSE 
		UPDATE PagerCallAlarm SET LastTime=@occtime WHERE id=@id;	
END
GO

/****** Object:  Table [dbo].[MattressStatus]    Script Date: 03/31/2015 11:19:17 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[MattressStatus](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[MattressId] [int] NOT NULL,
	[Status] [int] NOT NULL DEFAULT ((0)),
	[StartTime] [datetime] NOT NULL,
	[EndTime] [datetime] NULL,
 CONSTRAINT [PK_MattressStatus] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

/****** Object:  Table [dbo].[MattressStatusAlarm]    Script Date: 03/24/2015 14:24:50 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[MattressStatusAlarm](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[MattressId] [int] NOT NULL,
	[Status] [int] NOT NULL DEFAULT ((0)),
	[StartTime] [datetime] NOT NULL,
	[OkTime] [datetime] NULL,
	[IsRead] [int] NOT NULL DEFAULT ((0)),
 CONSTRAINT [PK_MattressStatusAlarm] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

/****** Object:  Table [dbo].[MattressAlarmSet]    Script Date: 04/01/2015 11:27:49 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[MattressAlarmSet](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[StartTime] [datetime] NOT NULL,
	[EndTime] [datetime] NOT NULL,
	[Status] [int] NOT NULL DEFAULT ((0)) ,
 CONSTRAINT [PK_MattressAlarmSet] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO


/****** Object: Proc SP_UpdateMattressAlarm    Script Date: 04/01/2015 11:27:49 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE SP_UpdateMattressAlarm
(
	@num INT,
	@status INT,
	@occtime DATETIME,
	@type INT = 0 -- 0 insert, 1 update
)
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
	
	IF @type = 1
	BEGIN
		update MattressStatusAlarm set OkTime=@occtime where MattressId=@num and OkTime is null;
		RETURN;
	END
	
	DECLARE @oktime DATETIME,
			@isread INT
	SET @isread = NULL;
	
	SELECT TOP 1 @oktime=OkTime, @isread=IsRead FROM dbo.MattressStatusAlarm WHERE MattressId=@num ORDER BY StartTime DESC;
	IF @isread IS NULL OR @oktime IS NOT NULL
		insert into MattressStatusAlarm(MattressId, Status, StartTime) values(@num, @status, @occtime);
    
END

GO

/****** Object:  StoredProcedure [dbo].[sp_InsertAlarmToDownMessage]    Script Date: 04/08/2015 14:04:53 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[sp_InsertAlarmToDownMessage]
(
	@alarmtype INT,   --1, 卡求救 2，呼救器 3，离床报警
	@cardnum NVARCHAR(50),
	@cardtype INT,
	@occtime DATETIME
)
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- Insert statements for procedure here
	DECLARE @name NVARCHAR(50), @nursers NVARCHAR(256);
	SET @name = NULL; 
	SET @nursers = NULL;	
	DECLARE @content NVARCHAR(256), @addr NVARCHAR(128);
	SET @content = NULL;
	SET @addr = NULL;
	IF @alarmtype=1
	BEGIN
		SELECT @name=p1.PersonName, @nursers=ISNULL(@nursers+',', '')+p2.CardNumber FROM dbo.Person p1 INNER JOIN dbo.NurseRelation n 
		ON p1.CardNumber=@cardnum AND p1.CardType=@cardtype AND p1.id=n.CustomerId INNER JOIN dbo.Person p2 ON n.NurseId=p2.id;
	
		IF @name IS NULL OR @nursers=''
			RETURN;
			
		SELECT @addr= CASE WHEN c.Distance <= 0 THEN N'在 ' + ISNULL(w.Address, CONVERT(NVARCHAR(10), w.WorkSiteId)) + N' 附近' 
					  ELSE N'距 ' + ISNULL(w.Address, CONVERT(NVARCHAR(10), w.WorkSiteId))  + N' ' + CONVERT(NVARCHAR(50),c.Distance) + N'米' 
					  END
		FROM dbo.CardLoc c INNER JOIN dbo.WorkSite w ON c.WorkSiteId=w.WorkSiteId 
		WHERE c.CardNum=@cardnum AND c.CardType=@cardtype;
		
		IF @addr IS NULL
		BEGIN
			SELECT @addr= CASE WHEN c.Distance <= 0 THEN N'在 ' + ISNULL(w.Address, CONVERT(NVARCHAR(10), w.WorkSiteId)) + N' 附近' 
				ELSE N'距 ' + ISNULL(w.Address, CONVERT(NVARCHAR(10), w.WorkSiteId))  + N' ' + CONVERT(NVARCHAR(50),c.Distance) + N'米' 
				END
			FROM dbo.CardLoc_2D c INNER JOIN dbo.WorkSite w ON c.WorkSiteId=w.WorkSiteId 
		WHERE c.CardNum=@cardnum AND c.CardType=@cardtype;
		END
		
		SET @content = @name + N',人员卡求救' + ISNULL(N','+@addr+N'.', N'.');
	END
	ELSE IF @alarmtype=2
	BEGIN
		SELECT @name=p1.PersonName, @nursers=ISNULL(@nursers+',', '')+p2.CardNumber, @addr= a.aName+t.Address 
		FROM dbo.Person p1 INNER JOIN dbo.Customer c ON p1.id=c.PersonId INNER JOIN dbo.NurseUnit t 
		ON c.NurseUnitId=t.id and t.PagerNum=@cardnum INNER JOIN dbo.Area a ON t.AreaId=a.id 
		INNER JOIN dbo.NurseRelation n ON p1.id=n.CustomerId INNER JOIN dbo.Person p2 ON n.NurseId=p2.id
	
		IF @name IS NULL OR @nursers=''
			RETURN;
			
		SET @content = @name + N',呼救器求救' + ISNULL(N','+@addr+N'.', N'.');
	END
	ELSE IF @alarmtype=3
	BEGIN
		SELECT @name=p1.PersonName, @nursers=ISNULL(@nursers+',', '')+p2.CardNumber, @addr= a.aName+t.Address 
		FROM dbo.Person p1 INNER JOIN dbo.Customer c ON p1.id=c.PersonId INNER JOIN dbo.NurseUnit t 
		ON c.NurseUnitId=t.id and t.MattressNum=@cardnum INNER JOIN dbo.Area a ON t.AreaId=a.id 
		INNER JOIN dbo.NurseRelation n ON p1.id=n.CustomerId INNER JOIN dbo.Person p2 ON n.NurseId=p2.id
	
		IF @name IS NULL OR @nursers=''
			RETURN;
			
		SET @content = @name + N',离床报警' + ISNULL(N','+@addr+N'.', N'.');
	END
	
	--PRINT @name + ' ' + @nursers + ' ' + @addr;
	INSERT INTO dbo.DownMessage(Type, SiteList, CardList, Content, OccTime) VALUES(@cardtype, '', @nursers, @content, @occtime);
END

GO

/****** Object:  Trigger [dbo].[Tr_CardSos_AfterInsert]    Script Date: 04/08/2015 14:43:23 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TRIGGER Tr_CardSos_AfterInsert
   ON  dbo.CardSos
   AFTER INSERT
AS 
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- Insert statements for trigger here
    DECLARE @cardnum NVARCHAR(50), @cardtype INT, @occtime DATETIME;
    SELECT @cardnum=CardNum, @cardtype=CardType, @occtime=OccTime FROM INSERTED;
    EXEC dbo.sp_InsertAlarmToDownMessage @alarmtype=1, @cardnum=@cardnum, @cardtype=@cardtype, @occtime=@occtime;
    
END

GO

/****** Object:  Trigger [dbo].[Tr_CardSos_AfterInsert]    Script Date: 06/09/2015 16:03:58 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TRIGGER Tr_CardSos_AfterUpdate
   ON  dbo.CardSos
   AFTER UPDATE
AS 
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
    -- Insert statements for trigger here
    IF UPDATE(LastTime)
    BEGIN
		DECLARE @oldtime DATETIME
		SET @oldtime = NULL;
		SELECT @oldtime=LastTime FROM DELETED;
		
		DECLARE @cardnum NVARCHAR(50), @cardtype INT, @occtime DATETIME;
		SELECT @cardnum=CardNum, @cardtype=CardType, @occtime=LastTime FROM INSERTED;
		IF @oldtime IS NOT NULL AND DATEDIFF(SECOND, @oldtime, @occtime) > 60
			EXEC dbo.sp_InsertAlarmToDownMessage @alarmtype=1, @cardnum=@cardnum, @cardtype=@cardtype, @occtime=@occtime;
    END
END

GO

/****** Object:  Trigger [dbo].[Tr_PagerCallAlarm_AfterInsert]    Script Date: 04/08/2015 14:43:23 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TRIGGER Tr_PagerCallAlarm_AfterInsert
   ON  dbo.PagerCallAlarm
   AFTER INSERT
AS 
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- Insert statements for trigger here
    DECLARE @pagernum NVARCHAR(50), @alarmtime DATETIME;
    SELECT @pagernum=PagerNum, @alarmtime=OccTime FROM INSERTED;
    EXEC dbo.sp_InsertAlarmToDownMessage @alarmtype=2, @cardnum=@pagernum, @cardtype=9, @occtime=@alarmtime;
    
END

GO

/****** Object:  Trigger [dbo].[Tr_MattressStatusAlarm_AfterInsert]    Script Date: 04/08/2015 14:43:23 ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE TRIGGER [dbo].[Tr_MattressStatusAlarm_AfterInsert]
   ON  [dbo].[MattressStatusAlarm]
   AFTER INSERT
AS 
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- Insert statements for trigger here
    DECLARE @mattressid NVARCHAR(50), @alarmtime DATETIME;
    SELECT @mattressid=MattressId, @alarmtime=StartTime FROM INSERTED;
    EXEC dbo.sp_InsertAlarmToDownMessage @alarmtype=3, @cardnum=@mattressid, @cardtype=8, @occtime=@alarmtime;
    
END

GO

/*==============================================================*/
/* Table: ArticlesStatus	物品状态						   */
/*==============================================================*/
create table    dbo.ArticlesStatus
(
	id					int		primary key IDENTITY(1,1)		NOT NULL,
	StatusId			int		not NULL,
	StatusName	nvarchar(50)	not null
)
go

/*==============================================================*/
/* Table: ArticlesType     									   */
/*==============================================================*/
create table	dbo.ArticlesType
(
	id					int		primary key IDENTITY(1,1)		NOT NULL,
	Name				nvarchar(50)		not null,
	parentId			int					null,
)
go

/*==============================================================*/
/* Table: Articles											   */
/*==============================================================*/
create table	dbo.Articles
(
	id					int			primary key IDENTITY(1,1)		NOT NULL,
	cardNumber			varchar(50)		NOT NULL unique,
	name				nvarchar(50)	COLLATE Chinese_PRC_CI_AS NOT NULL,
	type				int				NOT  NULL,
	Tag					varchar(50)		NULL,
	departmentId		int				NULL,
	RegisterTime		datetime		NULL, --设备登记时间
	destoryTime			datetime		null, --设备取消绑定时间
	TimeStamp			datetime		NULL, 
	CRC					int				NOT NULL,
	description			nvarchar(200)	NULL,
	photoExtends		varchar(10)		COLLATE Chinese_PRC_CI_AS NULL,
	photo				image			NULL,
)
go

/*==============================================================*/
/* Table: ArticleCardType                  					   */
/*==============================================================*/      
create table dbo.ArticleCardType
(
	typeId			int			not null,
	typeName		nvarchar(100)	not null
)
go


/*==============================================================*/
/* Table: DownCmd		下发命令表							   */
/*==============================================================*/      
 create table dbo.DownCmd
(
	id				int			primary key IDENTITY(1,1)		NOT NULL,
	CmdType			int			not null,	--下发命令类型
--	CardNumber		varchar(50)	NULL NULL,	--下发卡号
	CardNumberList	Text		null,		--下发命令所携带的信息，如果多张卡的时候用,分隔开
	cardCount		int			not null,	--如果多张卡时表示卡的数量
	Issuccess		int			not null,	--命令当前状态， 0 正常下发，1 已经下发， 2 下发前被取消
	AddTime			DateTime	not null,	--记录入表时间
	isWriteCard		int			not null default(0)	--写卡是否成功	 0 未知，1 成功， 2 失败
)
go

/*==============================================================*/
/* Table: CardTypeManager  								   */
/*==============================================================*/
create table dbo.CardTypeManager
(
	id				int				primary key not null,
	Name			nvarchar(50)	not null,
)
Go

/*==============================================================*/
/* Table: userLogin											   */
/*==============================================================*/
create table dbo.userLogin
(
	id				int				primary key identity(1,1) not null,
	userName		nvarchar(50)					not null unique,
	password		varchar(30)						not null,
	trueName		nvarchar(50)					null,
	--roleName		nvarchar(50)					not null,
	lastLoginTime	datetime						null,
	createDateTime	datetime						null,
	parentId		int								null,
	permissionId	int								null
--	typeManagerId	int								not null default(0), /* 所属科室	默认为0 可以是区域等类型*/
--	areaId			int								not null default(0), -- 查看管理区域下人员
)
go

/*==============================================================*/
/* Table: userDepartConfig									   */
/*==============================================================*/
create table dbo.userDepartConfig
(
	id				int				primary key identity(1,1) not null,
	userName		nvarchar(50)	not null ,
	departId		int				not null,
)
Go

/*==============================================================*/
/* Table: Role	角色表（admin  super administration  一般用户）	*/
/*==============================================================*/
create table dbo.Role
(
		id				int				primary key  not null,
		--RoleID			int				not null unique,
		RoleName		nvarchar(50)	null,	--角色名称
		RoleDescription nvarchar(200)	null,	--角色描述
		parentRoleID		int			null,	--父角色ID
		CardTypeManagerId	int			not null default(0)
		--CreateDatetime	datetime		null	--创建时间
)
go


/*==============================================================*/
/* Table: userRoleRelation										*/
/*==============================================================*/
create table dbo.userRoleRelation
(
	id				int				primary key identity(1,1) not null,
	userId			int				not null,	--用户id
	roleId			int				not null	--角色id
)
go

/*==============================================================*/
/* Table: actionType     										*/
/*==============================================================*/
create table dbo.actionType
(
	id				int				primary key not null,
	code			nvarchar(50)	not null,
	name			nvarchar(50)	not null
)
Go

/*==============================================================*/
/* Table: Module         										*/
/*==============================================================*/
create table dbo.Module
(
	id				int				primary key not null,
	code			nvarchar(50)	not null,
	name			nvarchar(50)	not null
)
Go


/*==============================================================*/
/* Table: action	权限										*/
/*==============================================================*/
create table dbo.action
(
		id				int				primary key not null,
		ActionName		nvarchar(30)	null,
		actionTypeId	int				not null,
		moduleId		int				not null
)
Go


/*==============================================================*/
/* Table: RoleActionRelation									*/
/*==============================================================*/
create table dbo.RoleActionRelation
(
	id				int				primary key identity(1,1) not null,
	RoleId			int				null,  --权限ID
	ActionId		int				null   --权限ID
)
go

/*==============================================================*/
/* Table: 换卡记录表	人换卡			   						*/
/*==============================================================*/
create table dbo.ChangeCardNum
(
	id		int		primary key identity(1,1) not null,
	RecordIdentity		int					not null,
	cardNumber			nvarchar(50)		not null,
	personName			nvarchar(50)		not null,
	personNo			nvarchar(50)		null,
 	Stime 				datetime	   		not null,
	Etime				datetime			not null
)
go

/*==============================================================*/
/* Table: HistoryPersonInfo		                                */
/*==============================================================*/
CREATE TABLE dbo.HistoryPerson
(
	id					int			primary key IDENTITY(1,1)		NOT NULL,
	cardNumber			varchar(50)		NOT NULL,
	personName			nvarchar(50)	COLLATE Chinese_PRC_CI_AS NOT NULL,
	personNo			varchar(50)		COLLATE Chinese_PRC_CI_AS NULL,
	officePositionId	int				NULL,
	classTeamId			int				NULL,
	departmentId		int				NULL,
	sex					nvarchar(7)		COLLATE Chinese_PRC_CI_AS NULL,
	mobile				varchar(30)		COLLATE Chinese_PRC_CI_AS NULL,
	telephone			varchar(30)		COLLATE Chinese_PRC_CI_AS NULL,
	identifyNum			varchar(20)		COLLATE Chinese_PRC_CI_AS NULL,
	photoExtends		varchar(10)		COLLATE Chinese_PRC_CI_AS NULL,
	photo				image				NULL,
	inTime		dateTime					NULL,
	outTime		dateTime					NULL,
	RecordIdentity		int					NOT NULL
)
go

/*==============================================================*/
/* Table: HistoryArticles	                                    */
/*==============================================================*/
create table dbo.HistoryArticles
(
	id					int			primary key IDENTITY(1,1)		NOT NULL,
	cardNumber			varchar(50)		NOT NULL,
	name				nvarchar(50)	COLLATE Chinese_PRC_CI_AS NOT NULL,
	type				int				NOT  NULL,
	Tag					varchar(50)		NULL,
	departmentId		int				NULL,
	RegisterTime		datetime		NULL, --设备登记时间
	destoryTime			datetime		null, --设备取消绑定时间
	TimeStamp			datetime		NULL, 
	CRC					int				NOT NULL,
	description			nvarchar(200)	NULL,
	photoExtends		varchar(10)		COLLATE Chinese_PRC_CI_AS NULL,
	photo				image			NULL,
	RecordIdentity		int				NOT NULL
)
Go
/*==============================================================*/
/* Table: 系统参数配置				   							*/
/*==============================================================*/
create table dbo.sysConfig
(
    sysTitle	nvarchar(50) not null,			---标题
	SignaloutTime int		 not null,			--- 秒
)

Go
/*==============================================================*/
/* Table: 操作日志					   							*/
/*==============================================================*/

CREATE TABLE [dbo].[operateLog](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[actor] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NOT NULL,
	[opThing] [text] COLLATE Chinese_PRC_CI_AS NOT NULL,
	[operatType] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NOT NULL,
	[description] [text] COLLATE Chinese_PRC_CI_AS NULL,
	[operateTime] [datetime] NOT NULL,
 CONSTRAINT [PK_opreateLog] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

------------------------------2D新增表结构-----------------------------------

/*==============================================================*/
/* Table: 角色表(Roles)			   	     						*/
/*==============================================================*/
create table Roles
(
   ID uniqueidentifier primary key not null,     --ID
   Name varchar(20) not null,                    --名称  
   ParentID uniqueidentifier not null,           --父ID   
   Disabled bit not null,                         --是否禁用
   PageID int identity(1,1) not null
)
go

/*==============================================================*/
/* Table: 系统登录用户表(Users)	  	     						*/
/*==============================================================*/
create table Users
(
   ID uniqueidentifier primary key not null,      --ID
   Code varchar(50) not null ,                    --用户登录Code
   Name varchar(50) not null,                     --用户名称
   Password varchar(50)  null,                    --用户密码
   CanLogin bit not null,                         --是否能登录
   Disabled bit not null,                         --是否禁用
   description varchar(50) null,                  --备注信息
   Modifier varchar(20) null,                     --最后修改人
   ModifyTime datetime null,                       --最后修改时间
   PageID int identity(1,1) not null
)
GO

/*==============================================================*/
/* Table: 用户角色(UserRole)	  	     						*/
/*==============================================================*/
create table UserRole
(
   UserID uniqueidentifier not null,              --用户ID
   RoleID uniqueidentifier not null,              --角色ID
   PageID int identity(1,1) not null
)
GO

/*==============================================================*/
/* Table: 在线用户(OnlineUsers)	  	     						*/
/*==============================================================*/
create table OnlineUsers
(
   LoginID uniqueidentifier primary key not null,    --ID
   UserID uniqueidentifier not null,                 --用户ID
   IP varchar(20) null,                              --登录IP
   ComputerName varchar(50) null,                    --登录电脑名称
   LoginTime datetime not null,                      --登录时间
   LastActiveTime datetime null,                      --最后活动时间
   PageID int identity(1,1) not null
)
GO

/*==============================================================*/
/* Table: 操作名称(Operate)	  	     			    			*/
/*==============================================================*/
create table Operate
(
   ID uniqueidentifier primary key not null,      --ID
   Name varchar(30) not null ,                    --操作名称
   PageID int identity(1,1) not null
)
GO

/*==============================================================*/
/* Table: 菜单表(MenuList)	  	     			    			*/
/*==============================================================*/
create table MenuList
(
   ID uniqueidentifier primary key not null,     --ID
   Name varchar(20) not null,                    --名称(跟ModuleName一致)
   ParentID uniqueidentifier  null,              --父ID也可为BizTitle表的ID
   SerialNum int not null,                       --序号
   IsBizModule bit not null,                     --是否为常用模块当值为0时，NameSpace和FormName为空，代表此级菜单没有模块
   NameSpace varchar(100) null,                  --命名空间
   ViewName varchar(50) null,                    --窗体名称
   IconResourceName varchar(50) null,            --正常菜单Ico名称
   HIconResourceName varchar(50) null,           --鼠标划过菜单时的Ico名称
   Disabled bit not null,                        --是否禁用
   Description varchar(30) null,                  --备注信息 
   PageID int identity(1,1) not null,
   OperateListID varchar(1000) null              --功能所具有的操作ID集合
)
GO

/*==============================================================*/
/* Table: 快捷菜单(MenuFavorite)	  	     		    	    */
/*==============================================================*/
create table MenuFavorite
(
   ID uniqueidentifier primary key not null,    --ID
   UserID uniqueidentifier not null,               --用户ID
   MenuListID uniqueidentifier not null,             --MenuListID
   PageID int identity(1,1) not null
)
GO

/*==============================================================*/
/* Table: 角色权限(RolePermission)	  	     		    	    */
/*==============================================================*/
create table RolePermission
(
   ID uniqueidentifier primary key not null,    --ID
   RoleID uniqueidentifier not null,            --角色ID
   MenuListID uniqueidentifier not null,          --模块ID
   OperateID uniqueidentifier not null          --操作ID
)
GO

/*==============================================================*/
/* Table: 职务级别(OfficePositionLevel)	  	     		    	*/
/*==============================================================*/
create table OfficePositionLevel
(
   id int identity(1,1) primary key not null,    --ID
   name varchar(20) not null                    --名称
)
GO
/*==============================================================*/
/* Table: 地图([Map])               	  	     		    	*/
/*==============================================================*/
CREATE TABLE [dbo].[Map](
	[Id] [int] IDENTITY(1,1) NOT NULL,
	[Name] [nvarchar](50) NULL,
	[Path] [nvarchar](200) NOT NULL,
	[ParentId] [int] NOT NULL,
	[MapOrder] [int] NULL,
	[IsDefault] [nvarchar](1) NOT NULL,
	[BeginX] [float] NULL,
	[BeginY] [float] NULL,
	[EndX] [float] NULL,
	[EndY] [float] NULL,
	[XinParent] [float] NULL,
	[YinParent] [float] NULL,
 CONSTRAINT [PK_Map] PRIMARY KEY CLUSTERED 
(
	[Id] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]
GO

/*==============================================================*/
/* Table: 撤离信息([EvacuateInfo])               	 	    	*/
/*==============================================================*/
CREATE TABLE [dbo].[EvacuateInfo]
(
    [ID] [int] IDENTITY(1,1) primary key NOT NULL,
	[AreaIDList] [varchar](200) null,
	[Type] [int] null, -- 0代表下达撤离,1代表取消撤离,2已完成
	[OccTime] [datetime] null,
	[CancelTime] [datetime] null
)
GO
/*==============================================================*/
/* Table: 撤离反馈信息([EvacuateFeedback])               	 	*/
/*==============================================================*/
CREATE TABLE [dbo].[EvacuateFeedback]
(
    [ID] [int] IDENTITY(1,1) primary key NOT NULL,
	[EvacuateInfoID] [int] null,
	[CardNumber] [varchar](20) null,
	[Type] [int] null, -- 0代表未确认,1代表已确认
	[ACKFirstTime] [datetime] null,
	[ACKLastTime] [datetime] null,
	[ConfirmFirstTime] [datetime] null,
	[ConfirmLastTime] [datetime] null

)
GO
/*==============================================================*/
/* Table: 区域经过表([AreaPass_2D])               	 	*/
/*==============================================================*/
CREATE TABLE [dbo].[AreaPass_2D](
	[Id] [int] IDENTITY(1,1) NOT NULL,
	[CardNumber] [nvarchar](50) NOT NULL,
	[AreaName] [nvarchar](50) NOT NULL,
	[ObjectId] [int] NOT NULL,
	[MapId] [int] NOT NULL,
	[OccTime] [datetime] NOT NULL,
	[X] [float] NOT NULL,
	[Y] [float] NOT NULL
) ON [PRIMARY]

GO

/*==============================================================*/
/* Table: 跳传链路表([WifiLinkInfo])               	 	*/
/*==============================================================*/
CREATE TABLE [dbo].[WifiLinkInfo](
	[CurrentID] [int] NOT NULL,
	[ParentID] [int] NOT NULL,
	[OccTime] [datetime] NULL,
 CONSTRAINT [PK_WifiLinkInfo] PRIMARY KEY CLUSTERED 
(
	[CurrentID] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO



/*==============================================================*/
/* Table: 跳传链路历史表([WifiLinkInfo])               	 	*/
/*==============================================================*/
CREATE TABLE [dbo].[WifiLinkHistory](
	[Id] [int] IDENTITY(1,1) NOT NULL,
	[CurrentID] [int] NOT NULL,
	[ParentID] [int] NOT NULL,
	[OccTime] [datetime] NOT NULL,
 CONSTRAINT [PK_WifiLinkHistory] PRIMARY KEY CLUSTERED 
(
	[Id] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO


/*==============================================================*/
/* Table: WristCardSilentAlarm                                  */
/*==============================================================*/
CREATE TABLE [dbo].[WristCardSilentAlarm](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[CardNum] [nvarchar](50) NOT NULL,
	[CardType] [int] NOT NULL,
	[OccTime] [datetime] NOT NULL,
	[OkTime] [datetime] NULL,
	[Msg] [nvarchar](256) NULL,
	[IsRead] [int] NOT NULL,
	[WorkSiteId] [int] NOT NULL,
 CONSTRAINT [PK_WristCardSilentAlarm] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

ALTER TABLE [dbo].[WristCardSilentAlarm] ADD  CONSTRAINT [DF_WristCardSilentAlarm_IsRead]  DEFAULT ((0)) FOR [IsRead]
GO

ALTER TABLE [dbo].[WristCardSilentAlarm] ADD  CONSTRAINT [DF_WristCardSilentAlarm_WorkSiteId]  DEFAULT ((0)) FOR [WorkSiteId]
GO



/*==============================================================*/
/* Table: WristFallAlarm                                        */
/*==============================================================*/
create table dbo.WristFallAlarm (
   id                   int                  identity(1, 1),
   CardNum              nvarchar(50)         collate Chinese_PRC_CI_AS not null,
   CardType				int					 not null,
   WorkSiteId           int                  not null constraint DF_WristFallAlarm_WorkSiteId default (0),
   OccTime              datetime             not null,
   IsRead               int                  not null constraint DF_WristFallAlarm_IsRead default (0),
   constraint PK_WristFallAlarm primary key (id)
         on "PRIMARY"
)
on "PRIMARY"
GO

/*==============================================================*/
/* Table: WristDisConAlarm                                        */
/*==============================================================*/
CREATE TABLE [dbo].[WristDisConAlarm](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[CardNum] [nvarchar](50) NOT NULL,
	[CardType] [int] NOT NULL,
	[WorkSiteId] [int] NOT NULL DEFAULT ((0)),
	[OccTime] [datetime] NOT NULL,
	[OkTime] [datetime]  NULL,
	[IsRead] [int] NOT NULL DEFAULT ((0)),
 CONSTRAINT [PK_WristDisConAlarm] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO



/*==============================================================*/
/* Table: DownMessage                                           */
/*==============================================================*/
create table dbo.DownMessage (
   id                   int                  identity(1, 1),
   Type                 int                  not null, --表示下发那种类型信息，用于扩展
   SiteList             nvarchar(256)        not null, --下发给指定基站，基站间用，分开
   CardList             nvarchar(256)        not null, --下发给指定的卡，卡号间用，分开
   Content              Text                 not null,
   OccTime              datetime             not null,
   Status               int                  not null constraint DF_DownMessage_Status default (0), --0 待下发，1 正在下发，2 下发完成
   constraint PK_DownMessage primary key (id)
         on "PRIMARY"
)
on "PRIMARY"
GO

/*==============================================================*/
/* Table: AlarmTimeSpan                                         */
/*==============================================================*/
create table dbo.AlarmTimeSpan (
   id                   int                  identity(1, 1),
   ObjectID             int                  not null, --Person表中对于的ID值
   BeginTime            datetime             not null,
   EndTime              datetime             not null,
   AlarmType            int                  not null,
   constraint PK_AlarmTimeSpan primary key (id)
         on "PRIMARY"
)
on "PRIMARY"
GO

CREATE TABLE [dbo].[activityInfo](
	[id]          [int]        IDENTITY(1,1) NOT NULL,
	[ObjectID]    [int]        NOT NULL,
	[time]        [datetime]   NULL CONSTRAINT [DF_activityInfo_time]  DEFAULT (getdate()),
	[percentage]  [float]      NULL CONSTRAINT [DF_activityInfo_percentage]  DEFAULT ((0)),
	[reliability] [float]      NULL CONSTRAINT [DF_activityInfo_reliability]  DEFAULT ((0)),
    CONSTRAINT [PK_activityInfo_1] PRIMARY KEY CLUSTERED ( [id] ASC)
    WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]
GO


/*==============================================================*/
/* Table: [Camera]                                              */
/*==============================================================*/
CREATE TABLE [dbo].[Camera](
	[id] [int] IDENTITY(1,1) PRIMARY KEY NOT NULL,
	[MacAddress] [varchar](30)  NULL,
	[IpAddress] [varchar](30) NOT NULL,
	[devName] [nvarchar](100) NOT NULL,
	[devType] [nvarchar](50) NULL,
	[AddressPos] [nvarchar](100) NULL,
	[PresetPosition] [varchar](100) NULL,
	[x_2d] [float] NULL,
	[y_2d] [float] NULL,
	[mapid] [int] NULL,
	[x_3d] [float] NULL,
	[y_3d] [float] NULL,
	[z_3d] [float] NULL,
	[rotaryW_3d] [float] NULL,
	[rotaryX_3d] [float] NULL,
	[rotaryY_3d] [float] NULL,
	[rotaryZ_3d] [float] NULL,
	[AreaId]	 [int]   NULL,	--摄像头区域id 用于查找地图所属分层
 )
GO
/*==============================================================*/
/* Table: CameraToWorkSiteMap                                   */
/*==============================================================*/
CREATE TABLE [dbo].[CameraToWorkSiteMap](
	[id] [int] IDENTITY(1,1) PRIMARY KEY  NOT NULL,
	[WorkSiteId] [int] NULL,
	[CameraId] [int] NULL,
	[PresetPos] [int] NULL
) 
GO
---------------------------------------------------Table End--------------------------------------------------------

--------------------------------------------------------View Begin----------------------------------------------


CREATE VIEW [dbo].[OperationLogView]
AS
SELECT     dbo.operateLog.id, dbo.operateLog.actor, dbo.operateLog.opThing, dbo.operateLog.operatType, dbo.OperateLogType.TypeName, dbo.operateLog.description, 
                      dbo.operateLog.operateTime
FROM         dbo.operateLog INNER JOIN
                      dbo.OperateLogType ON dbo.operateLog.operatType = dbo.OperateLogType.TypeValue


GO

Create view [dbo].[EmployeeView]
as 
Select  dbo.person.id, dbo.person.cardNumber, dbo.person.personName, dbo.Person.PersonNo  as number, dbo.person.officePositionId, dbo.person.classTeamId, 
                      dbo.person.departmentId, dbo.person.sex, dbo.person.mobile, dbo.person.telephone, dbo.person.identifyNum, dbo.person.photoExtends, dbo.person.photo, 
                      dbo.person.inTime, dbo.person.outTime, dbo.department.name AS departmentName, dbo.classTeam.name AS TeamName,dbo.officePosition.Name as officePositionName, dbo.person.birthday, dbo.person.cardType
from dbo.person left join dbo.Employee on dbo.person.id=dbo.Employee.PersonId
LEFT OUTER JOIN dbo.department ON dbo.Employee.departmentId = dbo.department.id 
LEFT OUTER JOIN dbo.classTeam ON dbo.Employee.classTeamId = dbo.classTeam.id AND dbo.department.id = dbo.classTeam.departId
LEFT OUTER JOIN dbo.officeposition on dbo.OfficePosition.id = dbo.Employee.officePositionId 
Go


Create view [dbo].[CustomerView]
as 
Select  dbo.person.id, dbo.person.cardNumber, dbo.person.personName, dbo.person.sex, dbo.person.mobile, dbo.person.telephone, 
dbo.person.identifyNum, dbo.person.photoExtends, dbo.person.photo, dbo.person.inTime, dbo.person.outTime, dbo.person.birthday,
dbo.person.cardType,dbo.NurseUnit.AreaId,dbo.Area.aName,dbo.NurseUnit.Address,dbo.NurseUnit.MattressNum,dbo.NurseUnit.PagerNum
from dbo.person inner join dbo.Customer on dbo.person.id=dbo.Customer.PersonId
INNER JOIN dbo.NurseUnit ON dbo.Customer.NurseUnitId = dbo.NurseUnit.id
LEFT OUTER JOIN dbo.Area ON dbo.Area.id = dbo.NurseUnit.AreaId
Go

--- dbo.CardLowPower.CardType = 3 人员卡 
--- dbo.CardLowPower.CardType = 6 腕带卡
--- dbo.CardLowPower.CardType = 10 胸牌卡
Create view [dbo].[cardLowPowerMsg_Vw] 
as
SELECT     dbo.CardLowPower.id, dbo.CardLowPower.CardNum, dbo.person.personName,dbo.CardLowPower.Power, 
					dbo.CardLowPower.OccTime, dbo.CardLowPower.InDBTime, 
                      dbo.CardLowPower.IsRead, dbo.WorkSite.Address,dbo.CardLowPower.CardType
FROM         dbo.CardLowPower INNER JOIN
                      dbo.person ON dbo.CardLowPower.CardNum = dbo.person.cardNumber
                      LEFT OUTER JOIN dbo.CardLoc ON dbo.CardLoc.CardNum = dbo.CardLowPower.CardNum  AND 
                      (dbo.CardLoc.CardType = 3 or dbo.CardLoc.CardType = 6 or dbo.CardLowPower.CardType = 10) LEFT OUTER JOIN
                      dbo.WorkSite ON dbo.WorkSite.WorkSiteId = dbo.CardLoc.WorkSiteId
go

Create view cardLowPowerMsg_goods_Vw 
as
select  dbo.CardLowPower.id,CardLowPower.cardNum,Articles.Tag,Articles.name as goodsName,at.name as goodsType,
Articles.departmentId,department.name AS department,CardLowPower.Power,CardLowPower.OccTime,CardLowPower.inDBTime,
CardLowPower.isRead,dbo.WorkSite.Address,dbo.Articles.RegisterTime
from CardLowPower inner join Articles on cardLowPower.cardNum=Articles.cardNumber and cardLowPower.cardType=4 
				 inner join dbo.CardInformation AS ci ON ci.CardNum = dbo.CardLowPower.CardNum AND ci.CardType = dbo.CardLowPower.CardType 
				left join dbo.CardLoc on dbo.CardLoc.CardNum=dbo.CardLowPower.CardNum and CardLoc.cardType=4 left outer join
         dbo.WorkSite on dbo.WorkSite.worksiteid=dbo.CardLoc.Worksiteid left outer join
      department on Articles.departmentId = department.id left join ArticlesType at on at.id = Articles.type
go

Create view cardHistoryLowPowerMsg_goods_Vw 
as
select  dbo.CardLowPower.id,CardLowPower.cardNum,HistoryArticles.Tag,HistoryArticles.name as goodsName,at.name as goodsType,
HistoryArticles.departmentId,department.name AS department,CardLowPower.Power,CardLowPower.OccTime,CardLowPower.inDBTime,
CardLowPower.isRead,dbo.WorkSite.Address,dbo.HistoryArticles.RegisterTime,dbo.HistoryArticles.destoryTime
from CardLowPower inner join HistoryArticles on cardLowPower.cardNum=HistoryArticles.cardNumber and cardLowPower.cardType=4 
				 inner join dbo.CardInformation AS ci ON ci.CardNum = dbo.CardLowPower.CardNum AND ci.CardType = dbo.CardLowPower.CardType 
				left join dbo.CardLoc on dbo.CardLoc.CardNum=dbo.CardLowPower.CardNum and CardLoc.cardType=4 left outer join
         dbo.WorkSite on dbo.WorkSite.worksiteid=dbo.CardLoc.Worksiteid left outer join
      department on HistoryArticles.departmentId = department.id left join ArticlesType at on at.id = HistoryArticles.type
go

/** customer Sos */
create view cardSosAlarmMsg_Vw
as
SELECT     s.id,s.CardNum,s.CardType,op.personName,dbo.NurseUnit.Address,dbo.NurseUnit.MattressNum,
           dbo.NurseUnit.PagerNum,s.OccTime,s.Lasttime,s.IsRead,worksite.address as wsAddress,area.aname,op.inTime,nu.personName nurseName
FROM         dbo.CardSos s INNER JOIN
              dbo.person op ON s.CardNum = op.cardNumber AND s.cardType = op.cardType
              and (s.cardType=3 or s.cardType=6 or s.CardType = 7) JOIN
              dbo.customer ON op.id = dbo.customer.PersonId LEFT OUTER JOIN 
              dbo.NurseUnit ON dbo.customer.NurseUnitId = dbo.NurseUnit.id LEFT OUTER JOIN 
			  dbo.worksite on dbo.worksite.worksiteid = s.worksiteid LEFT OUTER JOIN
			  dbo.corAreaworksite  on s.worksiteid=corAreaworksite.worksiteid LEFT OUTER JOIN
			  dbo.area on dbo.area.id=dbo.corAreaworksite.areaId LEFT OUTER JOIN
			  dbo.nurseRelation nr on customer.personId = nr.customerId LEFT OUTER JOIN
			  dbo.person nu on nr.nurseId = nu.id
go

CREATE View [dbo].[AreaAlarmMsg_Vw]
as
SELECT     dbo.areaAlarm.id,dbo.Area.id as areaId,dbo.Area.aName,dbo.AreaType.Name,dbo.areaAlarm.CardNum,dbo.areaAlarm.CardType,dbo.person.personName, 
           p.PersonName as nurseName,
           dbo.areaAlarm.beginTime,dbo.areaAlarm.IsRead, dbo.areaAlarm.endTime, dbo.WorkSite.Address as wsAddress,dbo.person.inTime
FROM         dbo.areaAlarm INNER JOIN dbo.person ON dbo.areaAlarm.CardNum = dbo.person.cardNumber 
					  AND dbo.areaAlarm.cardType = dbo.person.cardType
                      and (areaAlarm.cardType=3 or areaAlarm.cardType=6 or dbo.areaAlarm.CardType = 10) 
                      JOIN   dbo.customer c ON dbo.person.id = c.PersonId LEFT OUTER JOIN 
					  NurseRelation nr ON nr.CustomerId =  c.PersonId LEFT OUTER jOIN
					  Person p ON nr.NurseId = p.id LEFT OUTER JOIN 
                      dbo.WorkSite ON dbo.WorkSite.WorkSiteId = dbo.areaAlarm.WorkSiteNum LEFT OUTER JOIN
                      dbo.corAreaWorksite ON dbo.areaAlarm.WorkSiteNum = dbo.corAreaWorksite.WorksiteId LEFT OUTER JOIN
                      dbo.Area on dbo.Area.id=dbo.areaAlarm.workAreaId LEFT OUTER JOIN
					  dbo.AreaType ON dbo.Area.type = dbo.areaType.id 
go

CREATE View [dbo].[AreaOverTimeAlarmMsg_Vw]
as
SELECT     dbo.areaOverTimeAlarm.id,dbo.areaOverTimeAlarm.areaId,dbo.Area.aName,dbo.AreaType.Name,dbo.areaOverTimeAlarm.CardNum,dbo.person.personName,
		   dbo.NurseUnit.Address,dbo.NurseUnit.MattressNum,dbo.NurseUnit.PagerNum,
		   dbo.areaOverTimeAlarm.inTime,dbo.areaOverTimeAlarm.beginTime, dbo.areaOverTimeAlarm.IsRead,
		   dbo.areaOverTimeAlarm.endTime,dbo.WorkSite.Address as wsAddress,dbo.person.inTime as PersonInTime,p.PersonName as nurseName
FROM       dbo.areaOverTimeAlarm INNER JOIN dbo.person ON dbo.areaOverTimeAlarm.CardNum = dbo.person.cardNumber 
		   and (areaOverTimeAlarm.cardType=3 or areaOverTimeAlarm.cardType=6 ) 
		   JOIN dbo.customer ON dbo.person.id = dbo.customer.PersonId LEFT OUTER JOIN 
		   dbo.NurseUnit ON dbo.customer.NurseUnitId = dbo.NurseUnit.id LEFT OUTER JOIN 
           dbo.WorkSite ON dbo.WorkSite.WorkSiteId = dbo.areaOverTimeAlarm.WorkSiteNum LEFT OUTER JOIN
           dbo.corAreaWorksite ON dbo.areaOverTimeAlarm.WorkSiteNum = dbo.corAreaWorksite.WorksiteId LEFT OUTER JOIN
           dbo.Area on dbo.Area.id=dbo.areaOverTimeAlarm.AreaId LEFT OUTER JOIN
		   dbo.AreaType ON dbo.Area.type = dbo.areaType.id LEFT JOIN
		   dbo.NurseRelation on dbo.Person.id = dbo.NurseRelation.CustomerId LEFT JOIN
		   dbo.Person P ON dbo.NurseRelation.NurseId = P.id
go

create  view NoSignal_Person_AlarmMsg_Vw
as
	select ce.id,cl.CardNum,p.personName,dbo.NurseUnit.Address,dbo.NurseUnit.MattressNum,dbo.NurseUnit.PagerNum,ws.address as wsAddress,
	convert(varchar(23),ce.occTime,120)as occTime,convert(varchar(23),ce.okTime,120)as
	okTime,isRead from CardException ce inner join CardLoc cl on ce.cardNum=cl.cardNum and 
	(cl.cardType=3 or cl.cardType=6 or cl.CardType = 10)and (ce.cardType =3 or ce.cardType =6 or ce.CardType = 10) 
	inner join person p on p.cardNumber = cl.cardNum 	
	LEFT OUTER JOIN dbo.customer ON p.id = dbo.customer.PersonId 
	LEFT OUTER JOIN dbo.NurseUnit ON dbo.customer.NurseUnitId = dbo.NurseUnit.id 
	LEFT OUTER JOIN worksite ws on ws.worksiteid=cl.worksiteid 
	--where ce.occTime>p.inTime
Go

CREATE view [dbo].[RealTimePersonList_Vw]
as
select p.id, p.cardNumber, p.CardType, p.personName, ws.worksiteid, ws.address, a.id as areaId, a.aName, c.occtime, (CASE WHEN p.DepartmentId IS NULL OR p.DepartmentId<=0 THEN 1 ELSE 0 END) iscustomer
from cardLoc c inner join person p on p.cardNumber=c.cardNum AND p.CardType=c.CardType left join worksite ws on ws.worksiteid=c.worksiteid 
left join corAreaWorkSite caw on ws.worksiteid=caw.worksiteid left join area a on a.id=caw.areaid 
WHERE c.CardType IN (3,6,10)
GO

CREATE view [dbo].[RealTimeEmployeeList_Vw]
as
select person.id,person.cardNumber,person.personNo,person.personName,area.aName,ws.address,cl.occtime,ws.worksiteid,area.id as areaId,
dbo.Employee.officePositionId, dbo.Employee.classTeamId,dbo.Employee.departmentId,
dbo.department.name AS departmentName, dbo.classTeam.name AS TeamName,dbo.officePosition.Name as officePositionName
from cardLoc cl inner join person on person.cardNumber= cl.cardNum and (cl.CardType = 3 or cl.CardType = 6 or cl.CardType = 10) 
LEFT OUTER JOIN worksite ws on ws.worksiteid=cl.worksiteid left join corAreaWorkSite caw 
on ws.worksiteid=caw.worksiteid left join area on area.id=caw.areaid
INNER JOIN dbo.Employee on dbo.Employee.personid=dbo.person.id 
LEFT OUTER JOIN dbo.department ON dbo.Employee.departmentId = dbo.department.id 
LEFT OUTER JOIN dbo.classTeam ON dbo.Employee.classTeamId = dbo.classTeam.id AND dbo.department.id = dbo.classTeam.departId
LEFT OUTER JOIN dbo.officeposition on dbo.OfficePosition.id = dbo.Employee.officePositionId
GO

CREATE view [dbo].[RealTimeCustomerList_Vw]
as
select person.id,person.cardNumber,person.personName,nurse.personName as nurseName,area.aName,ws.address as wsAddress,cl.occtime,ws.worksiteid,area.id as areaId,
dbo.NurseUnit.Address,dbo.NurseUnit.MattressNum,dbo.NurseUnit.PagerNum
from cardLoc cl inner join person on person.cardNumber= cl.cardNum and (cl.CardType = 3 or cl.CardType = 6 or cl.CardType = 10)
LEFT OUTER JOIN worksite ws on ws.worksiteid=cl.worksiteid 
LEFT OUTER JOIN corAreaWorkSite caw ON ws.worksiteid=caw.worksiteid left join area on area.id=caw.areaid 
JOIN dbo.customer ON dbo.person.id = dbo.customer.PersonId 
LEFT OUTER JOIN dbo.NurseUnit ON dbo.customer.NurseUnitId = dbo.NurseUnit.id 
LEFT OUTER JOIN nurseRelation nr on nr.customerId = person.id
LEFT OUTER JOIN person nurse on nr.nurseId = nurse.id
GO

Create view [dbo].[RealTimeGoodsList_Vw]
as
select articles.id,articles.cardNumber,articles.Name,articles.type,articles.Tag,articles.departmentId,articles.RegisterTime,
dt.Name as DepartmentName,area.aName,ws.address,cl.occtime,ws.worksiteid,area.id as areaId,
at.Name as goodType,aStatus.StatusName as StatusName
from cardLoc cl inner join CardInformation ci on cl.cardNum=ci.cardNum inner join
articles on articles.cardNumber= cl.cardNum and cl.CardType = 4 AND ci.CardType = 4
LEFT join  ArticlesType at on at.id=articles.type 
left join  ArticlesStatus aStatus on ci.Status=aStatus.StatusId
left outer join department dt on articles.departmentId =dt.id  
left join worksite ws on ws.worksiteid=cl.worksiteid left join corAreaWorkSite caw 
on ws.worksiteid=caw.worksiteid left join area on area.id=caw.areaid 
GO

--------------------------------------------2D新增视图-------------------------------------------------
--------------------------------User_View-----------------------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO
CREATE VIEW Users_View
as
select U.*,r.ID as RoleID,r.Name as RoleName
from Users as U
left join UserRole as ur on u.ID = ur.UserID
left join Roles as r on ur.RoleID = r.ID
go
--select *from Roles
--select * from UserRole
----------------------------------------------------------------------

--------------------------------Role_View------------------------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO
CREATE VIEW [dbo].[Role_View]
AS
SELECT dbo.Roles.*, ISNULL(FatherRole.Name, '') AS ParentRoleName
FROM dbo.Roles LEFT OUTER JOIN
      dbo.Roles FatherRole ON dbo.Roles.ParentID = FatherRole.ID
go
-------------------------------------------------------------------------

----------------------------------OnlineUsers_View-----------------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[OnlineUsers_View]
AS
SELECT dbo.Users.Code AS UserCode, 
      dbo.Users.Name AS UsersName, dbo.OnlineUsers.*
FROM dbo.OnlineUsers INNER JOIN
      dbo.Users ON dbo.OnlineUsers.UserID = dbo.Users.ID 

GO
---------------------------------------------------------------------------

-------------------------------------MenuFavorite_View----------------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[MenuFavorite_View]
AS
SELECT dbo.MenuFavorite.*,dbo.Users.Name as UserName,dbo.Users.Code as UserCode ,dbo.MenuList.Name AS MenuName, 
      dbo.MenuList.ParentID AS ParentID, dbo.MenuList.SerialNum AS SerialNum, 
      dbo.MenuList.IsBizModule AS IsBizModule, 
      dbo.MenuList.NameSpace AS NameSpace, 
      dbo.MenuList.ViewName AS ViewName,
	  dbo.MenuList.IconResourceName As IconResourceName,
	  dbo.MenuList.HIconResourceName  As HIconResourceName,
	  dbo.MenuList.Disabled AS Disabled
FROM dbo.MenuFavorite INNER JOIN
      dbo.MenuList ON dbo.MenuFavorite.MenuListID = dbo.MenuList.ID
	  INNER JOIN dbo.Users ON dbo.Users.ID = dbo.MenuFavorite.UserID
GO
--Select *from MenuFavorite_View
-----------------------------------------------------------------

-------------------------------------MenuList_View----------------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[MenuList_View]
AS
SELECT dbo.MenuList.*, FatherMenuList.Name AS ParentName
FROM dbo.MenuList LEFT OUTER JOIN
      dbo.MenuList FatherMenuList ON dbo.MenuList.ParentID = FatherMenuList.ID
GO
--Select *from [MenuList_View]
-----------------------------------------------------------------

-------------------------------------Department_View----------------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[Department_View]
AS
SELECT dbo.department.*, FatherDepartment.Name AS ParentName
FROM dbo.department LEFT OUTER JOIN
      dbo.department FatherDepartment ON dbo.department.ParentID = FatherDepartment.ID
GO
--Select *from [Department_View]
-----------------------------------------------------------------

------------------------------RolePermission_View----------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[RolePermission_View]
AS
SELECT dbo.Roles.Name AS RoleName, dbo.Operate.Name AS OperateName, 
      dbo.MenuList.Name AS ModuleName, dbo.RolePermission.*
FROM dbo.RolePermission INNER JOIN
      dbo.MenuList ON 
      dbo.RolePermission.MenuListID = dbo.MenuList.ID INNER JOIN
      dbo.Operate ON dbo.RolePermission.OperateID = dbo.Operate.ID INNER JOIN
      dbo.Roles ON dbo.RolePermission.RoleID = dbo.Roles.ID
	  where dbo.Roles.Disabled=0

GO
--select *from [RolePermission_View]
-----------------------------------------------------------------------

-----------------------------------------UserPermission_View---------------------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[UserPermission_View]
AS
SELECT DISTINCT 
      dbo.UserRole.UserID, dbo.MenuList.Name AS ModuleName, 
      dbo.Operate.Name AS OperateName
FROM dbo.UserRole 
INNER JOIN dbo.RolePermission ON dbo.UserRole.RoleID = dbo.RolePermission.RoleID 
INNER JOIN dbo.MenuList ON dbo.RolePermission.MenuListID = dbo.MenuList.ID 
INNER JOIN dbo.Operate ON dbo.RolePermission.OperateID = dbo.Operate.ID
INNER JOIN dbo.Roles ON dbo.ROles.ID = dbo.RolePermission.RoleID
where dbo.Roles.Disabled =0 

GO
--select *from UserPermission_View



-------------------------------------------------------------------------------

--------------------------------------Area_View---------------------------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[Area_View]
AS
select Area.id,Area.number,Area.aName,Area.Type,Area.parentId,Area.isAllow,Area.outSpanTime,
Area.personSize,count(AreaPurview.id)as PurviewCount,
AreaType.Name as typeName,p.aName as ParentName,Area.x,Area.y,dbo.Area.RssiThs
from Area 
left join AreaPurview on AreaPurview.AreaId = Area.id
left join AreaType on AreaType.id = Area.Type
left join Area p on p.id = Area.ParentID
group by area.id,area.number,area.aName,area.Type,area.Mode,
area.parentId,area.isAllow,area.outSpanTime,area.personSize,
AreaType.Name,p.aName,area.x,area.y,dbo.Area.RssiThs
GO
------------------------------------------------------------------

-----------------------------------------------[PersonInfo_View]-------------------------
CREATE VIEW [dbo].[PersonInfo_View]
AS
SELECT     empVw.id, empVw.cardNumber, empVw.personName,empVw.number as personNo, empVw.officePositionId, empVw.classTeamId, 
                      empVw.departmentId, empVw.sex, empVw.mobile, empVw.telephone, empVw.identifyNum, empVw.photoExtends, empVw.photo, 
                      empVw.inTime, empVw.outTime, dbo.department.name AS departmentName, dbo.classTeam.name AS TeamName,
                      dbo.officePosition.Name as officePositionName, empVw.birthday, empVw.cardType,officePosition.isNurse  
FROM         dbo.[EmployeeView] empVw left JOIN
                      dbo.department ON empVw.departmentId = dbo.department.id left JOIN
                      dbo.classTeam ON empVw.classTeamId = dbo.classTeam.id AND dbo.department.id = dbo.classTeam.departId
					  LEFT OUTER JOIN dbo.officeposition on dbo.OfficePosition.id = empVw.officePositionId 


GO


create view [dbo].[CarePersonName_view]
as
select Person.PersonName,Person.PersonNo,Person.CardNumber,NurseRelation.CustomerId
from NurseRelation left join Person 
on Person.id=NurseRelation.NurseId 
GO



create view [dbo].[OldPerson_view]
as
select Person.id as Id ,Person.CardNumber,Person.CardType,
Person.PersonNo as OldPersonNo,Person.PersonName as OldPersonName,Area.aName+NurseUnit.Address as DetailAddress,
NurseUnit.Address as CareLocation,Person.Sex ,Person.Mobile,Person.IdentifyNum,
 CarePersonName,Person.PhotoExtends,Person.Photo as photo, Person.InTime as inTime, 
 Person.OutTime as outTime,Area.aName,NurseUnit.AreaId,Customer.NurseUnitId from
 Person left join Customer on Person.id=Customer.PersonId left join NurseUnit on
 Customer.NurseUnitId=NurseUnit.id  left join Area on Area.id=NurseUnit.AreaId
 left join (SELECT  DISTINCT [customerid],STUFF((SELECT ','+[personname]
 FROM (select customerid,personname,PersonNo,CardNumber from CarePersonName_view) b WHERE customerid = A.customerid
 FOR XML PATH('')) ,1,1,'')AS CarePersonName
 FROM (select customerid,personname,PersonNo,CardNumber from CarePersonName_view) AS A) 
 a on Person.id=a.CustomerId  where Person.PersonNo='' or Person.PersonNo is null

GO



create view [dbo].[OldPerson_view_2D]
as
select Person.id as Id ,Person.CardNumber,Person.CardType as cardType,Person.ClassTeamId,Person.DepartmentId,Person.OfficePositionId,Person.Telephone,
Person.Birthday,Area.aName+NurseUnit.Address as DetailAddress,
Person.PersonNo ,Person.PersonName ,Area.aName,Customer.NurseUnitId,
NurseUnit.Address as CareLocation,isnull(NurseUnit.MattressNum,'') as MattressNum,
isnull(NurseUnit.PagerNum,'') as PagerNum,Person.Sex ,Person.Mobile,Person.IdentifyNum,
 CarePersonName,Person.PhotoExtends,Person.Photo as Photo, Person.InTime , NurseUnit.AreaId,
 Person.OutTime  from
 Person left join Customer on Person.id=Customer.PersonId left join NurseUnit on
 Customer.NurseUnitId=NurseUnit.id left join Area on Area.id=NurseUnit.AreaId
 left join (SELECT  DISTINCT [customerid],STUFF((SELECT ','+[personname]
 FROM (select customerid,personname,PersonNo,CardNumber from CarePersonName_view) b WHERE customerid = A.customerid
 FOR XML PATH('')) ,1,1,'')AS CarePersonName
 FROM (select customerid,personname,PersonNo,CardNumber from CarePersonName_view) AS A) 
 a on Person.id=a.CustomerId  where Person.PersonNo='' or Person.PersonNo is null

GO


CREATE view [dbo].[AllPerson_view_2D]
as
select Person.id as Id ,Person.CardNumber,Person.CardType as cardType,Person.ClassTeamId,Person.DepartmentId,
Person.OfficePositionId,Person.Telephone,classTeam.name as TeamName,
Person.Birthday,department.name as DepartmentName,officePosition.name as officePositionName,
Person.PersonNo ,Person.PersonName , officePosition.isNurse,
  Area.aName,Customer.NurseUnitId,
NurseUnit.Address as CareLocation,isnull(NurseUnit.MattressNum,'') as MattressNum,
isnull(NurseUnit.PagerNum,'') as PagerNum,Person.Sex ,Person.Mobile,Person.IdentifyNum,
 CarePersonName,Person.PhotoExtends,Person.Photo as Photo, Person.InTime , NurseUnit.AreaId,
 Person.OutTime from
 Person left join department on Person.DepartmentId=department.id
 left join officePosition on Person.OfficePositionId=officePosition.id
 left join classTeam on Person.ClassTeamId=classTeam.id left join Customer on Person.id=Customer.PersonId left join NurseUnit on
 Customer.NurseUnitId=NurseUnit.id left join Area on Area.id=NurseUnit.AreaId
 left join (SELECT  DISTINCT [customerid],STUFF((SELECT ','+[personname]
 FROM (select customerid,personname,PersonNo,CardNumber from CarePersonName_view) b WHERE customerid = A.customerid
 FOR XML PATH('')) ,1,1,'')AS CarePersonName
 FROM (select customerid,personname,PersonNo,CardNumber from CarePersonName_view) AS A) a on Person.id=a.CustomerId 


GO


 CREATE view [dbo].[CarePersonInfo_view_2d]
 as
 select NurseUnitId,Person.PersonName,P.PersonName as CarePersonName
 from Customer left join Person on Customer.PersonId=Person.id 
 left join NurseRelation on NurseRelation.CustomerId=Customer.PersonId 
 left join Person as P on P.id=NurseRelation.NurseId 

GO


CREATE view [dbo].[OldPersonLocationInfo_view_2D]
as
select PersonName ,Area.aName,NurseUnit.id ,
NurseUnit.Address ,isnull(NurseUnit.MattressNum,'') as MattressNum,
isnull(NurseUnit.PagerNum,'') as PagerNum,
 CarePersonName, NurseUnit.AreaId from
 NurseUnit inner join Area on Area.id=NurseUnit.AreaId
 left join ( select distinct(NurseUnitId),STUFF((SELECT ','+[personname]
 FROM (select distinct(NurseUnitId),personname from CarePersonInfo_view_2d) b WHERE NurseUnitId = A.NurseUnitId
 FOR XML PATH('')) ,1,1,'')AS personName,STUFF((SELECT ','+[CarePersonName]
 FROM (select distinct(NurseUnitId),CarePersonName from CarePersonInfo_view_2d) b WHERE NurseUnitId = A.NurseUnitId
 FOR XML PATH('')) ,1,1,'')AS CarePersonName from CarePersonInfo_view_2d A ) 
 a on NurseUnit.id=a.NurseUnitId  where   NurseUnit.id is not null




GO

------------------------------------------------------------------------------------------

--------------------------------------AreaPurview_View---------------------------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[AreaPurview_View]
AS
SELECT     ap.id, ap.areaId, ap.objectid, pv.personName, pv.departmentName, pv.cardNumber,pv.aName+pv.CareLocation as CareLocation,pv.CarePersonName
FROM         dbo.areaPurview AS ap LEFT OUTER JOIN
                      dbo.AllPerson_view_2D AS pv ON ap.objectid = pv.id 
GO

------------------------------------------------------------------

---------------------------------------------------workSite_View----------------------------
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[WorkSite_View]
AS
 select w.*,a.aName as AreaName,a.id as areaid,isnull(r.RssiFixValue,0) as RssiFixValue
 from workSite w 
 left join corAreaWorkSite aw on aw.workSiteId = w.workSiteId
 left join Area a on a.id = aw.areaId
 left join RssiFix r on r.WorksiteId = w.WorksiteId
 where w.workSiteId between 1000 and 65535
GO
--select *from workSite_View


--------------------------------------------------------------------------------------------

---------------------------------------------------LinkInfo_View----------------------------
CREATE VIEW [dbo].[LinkInfo_View]
AS
 select li.*,w.Address
 from linkInfo li
 left join workSite w on w.WorkSiteID = li.WorkSiteID
 where w.workSiteID between 1 and 999
GO
--select *from LinkInfo_View

-------------------------------------------------------------------------------------------------------

CREATE VIEW [dbo].[AreaAlarmMsg_Vw_2D]
AS
SELECT     dbo.areaAlarm.id, dbo.Area.id AS areaId, dbo.Area.aName, dbo.areaType.Name AS ATypeName, dbo.areaAlarm.cardNum, empvw.personName, 
                      empvw.departmentId, empvw.classTeamId, empvw.officePositionId,  
                    dbo.areaAlarm.beginTime, dbo.areaAlarm.isRead, dbo.areaAlarm.endTime, dbo.WorkSite.Address, empvw.inTime, 
                      dbo.areaType.id AS ATypeID,empvw.CarePersonName
FROM         dbo.areaAlarm INNER JOIN
                      dbo.OldPerson_view_2D as empvw ON dbo.areaAlarm.cardNum = empvw.cardNumber AND (dbo.areaAlarm.CardType = 3 or dbo.areaAlarm.CardType = 6) LEFT OUTER JOIN
                      dbo.WorkSite ON dbo.WorkSite.WorkSiteId = dbo.areaAlarm.workSiteNum LEFT OUTER JOIN
                      dbo.corAreaWorksite ON dbo.areaAlarm.workSiteNum = dbo.corAreaWorksite.WorksiteId LEFT OUTER JOIN
                      dbo.Area ON dbo.Area.id = dbo.areaAlarm.workAreaId LEFT OUTER JOIN
                      dbo.areaType ON dbo.Area.Type = dbo.areaType.id 


GO

CREATE View [dbo].[AreaOverTimeAlarmMsg_Vw_2D]
as
SELECT     dbo.areaOverTimeAlarm.id,dbo.areaOverTimeAlarm.areaId,dbo.Area.aName,dbo.AreaType.Name,dbo.areaOverTimeAlarm.CardNum, 
		   empvw.personName,empvw.PersonNo, empvw.departmentId, empvw.classTeamId, empvw.officePositionId, 
             empvw.CarePersonName,
		   dbo.areaOverTimeAlarm.inTime,dbo.areaOverTimeAlarm.beginTime, dbo.areaOverTimeAlarm.IsRead,
		   dbo.areaOverTimeAlarm.endTime,dbo.WorkSite.Address,empvw.inTime as PersonInTime
FROM       dbo.areaOverTimeAlarm INNER JOIN dbo.OldPerson_view_2D as empvw ON dbo.areaOverTimeAlarm.CardNum = empvw.cardNumber 
		   and (areaOverTimeAlarm.cardType=3 or areaOverTimeAlarm.cardType=6) LEFT OUTER JOIN
           dbo.WorkSite ON dbo.WorkSite.WorkSiteId = dbo.areaOverTimeAlarm.WorkSiteNum LEFT OUTER JOIN
           dbo.corAreaWorksite ON dbo.areaOverTimeAlarm.WorkSiteNum = dbo.corAreaWorksite.WorksiteId LEFT OUTER JOIN
           dbo.Area on dbo.Area.id=dbo.areaOverTimeAlarm.AreaId LEFT OUTER JOIN
		   dbo.AreaType ON dbo.Area.Type = dbo.areaType.id 




GO

CREATE VIEW [dbo].[Article_View_2D]
AS
SELECT     dbo.Articles.id, dbo.Articles.cardNumber, dbo.Articles.name, dbo.Articles.type, dbo.Articles.Tag, dbo.Articles.departmentId, dbo.Articles.RegisterTime, 
                      dbo.Articles.destoryTime, dbo.Articles.TimeStamp, dbo.Articles.CRC, dbo.Articles.description, dbo.Articles.photoExtends, dbo.Articles.photo, 
                      dbo.ArticlesType.Name AS TypeName, dbo.ArticlesStatus.StatusName, dbo.department.name AS DeparmentName, dbo.CardInformation.Status AS StatusId, 
                      dbo.ArticlesType.parentId AS ParentTypeId
FROM         dbo.Articles INNER JOIN
                      dbo.CardInformation ON dbo.CardInformation.CardNum = dbo.Articles.cardNumber INNER JOIN
                      dbo.ArticlesType ON dbo.Articles.type = dbo.ArticlesType.id INNER JOIN
                      dbo.ArticlesStatus ON dbo.CardInformation.Status = dbo.ArticlesStatus.StatusId INNER JOIN
                      dbo.department ON dbo.Articles.departmentId = dbo.department.id

GO



CREATE VIEW [dbo].[ArticleCardLoc_View_2D]
AS
SELECT     dbo.CardLoc_2D.CardNum, dbo.CardLoc_2D.CardType, dbo.CardLoc_2D.WorkSiteId, dbo.CardLoc_2D.x, dbo.CardLoc_2D.y, dbo.CardLoc_2D.OccTime, dbo.Articles.id, 
                      dbo.Articles.cardNumber, dbo.Articles.name, dbo.Articles.type, dbo.Articles.Tag, dbo.Articles.departmentId, dbo.Articles.RegisterTime, dbo.Articles.destoryTime, 
                      dbo.Articles.TimeStamp, dbo.Articles.CRC, dbo.Articles.description, dbo.Articles.photoExtends, dbo.Articles.photo, dbo.ArticlesType.Name AS TypeName, 
                      dbo.CardInformation.Status, dbo.CardInformation.InDBTime, dbo.CardInformation.Power, dbo.CardInformation.Version, dbo.ArticlesStatus.StatusName, 
                      dbo.department.name AS DepartmentName, dbo.WorkSite.mapid, dbo.WorkSite.Address, dbo.WorkSite.Type AS WorkSiteType, dbo.Map.Name AS MapName, 
                      dbo.Map.Path, dbo.Map.ParentId
FROM         dbo.CardLoc_2D INNER JOIN
                      dbo.Articles ON dbo.Articles.cardNumber = dbo.CardLoc_2D.CardNum INNER JOIN
                      dbo.ArticlesType ON dbo.Articles.type = dbo.ArticlesType.id INNER JOIN
                      dbo.CardInformation ON dbo.CardLoc_2D.CardNum = dbo.CardInformation.CardNum INNER JOIN
                      dbo.ArticlesStatus ON dbo.CardInformation.Status = dbo.ArticlesStatus.StatusId INNER JOIN
                      dbo.department ON dbo.Articles.departmentId = dbo.department.id INNER JOIN
                      dbo.WorkSite ON dbo.CardLoc_2D.WorkSiteId = dbo.WorkSite.WorkSiteId INNER JOIN
                      dbo.Map ON dbo.WorkSite.mapid = dbo.Map.Id
WHERE     (dbo.CardLoc_2D.CardType = 4)

GO


CREATE VIEW [dbo].[ArticleHistroyPath_View_2D]
AS
SELECT     TOP (100) PERCENT dbo.CardLoc_Interval_2D.id, dbo.CardLoc_Interval_2D.CardNum, dbo.CardLoc_Interval_2D.CardType, dbo.CardLoc_Interval_2D.WorkSiteId, 
                      dbo.CardLoc_Interval_2D.x, dbo.CardLoc_Interval_2D.y, dbo.CardLoc_Interval_2D.OccTime, dbo.CardLoc_Interval_2D.IsOnWorksite, 
                      dbo.CardLoc_Interval_2D.Objectid, dbo.Articles.name, dbo.Articles.Tag AS No, dbo.WorkSite.mapid, dbo.Map.Name AS MapName, dbo.Map.Path, 
                      dbo.Map.ParentId
FROM         dbo.CardLoc_Interval_2D INNER JOIN
                      dbo.Articles ON dbo.CardLoc_Interval_2D.Objectid = dbo.Articles.id INNER JOIN
                      dbo.WorkSite ON dbo.CardLoc_Interval_2D.WorkSiteId = dbo.WorkSite.WorkSiteId INNER JOIN
                      dbo.Map ON dbo.WorkSite.mapid = dbo.Map.Id
WHERE     (dbo.CardLoc_Interval_2D.CardType = 4)
ORDER BY dbo.CardLoc_Interval_2D.OccTime

GO

CREATE VIEW [dbo].[ArticleType_View_2D]
AS
SELECT     dbo.ArticlesType.id, dbo.ArticlesType.Name, dbo.ArticlesType.parentId, '无上级设备' AS ParentName
FROM         dbo.ArticlesType where .ArticlesType.parentId='-1'
union all
SELECT     dbo.ArticlesType.id, dbo.ArticlesType.Name, dbo.ArticlesType.parentId, ArticlesType_1.Name AS ParentName
FROM         dbo.ArticlesType INNER JOIN
                      dbo.ArticlesType AS ArticlesType_1 ON dbo.ArticlesType.parentId = ArticlesType_1.id

GO


CREATE VIEW [dbo].[CardLocMapView_2D]
AS
SELECT     dbo.WorkSite.mapid, dbo.WorkSite.Type, dbo.WorkSite.Address, dbo.Map.Name, dbo.Map.Path, dbo.Map.ParentId, dbo.Map.MapOrder, dbo.Map.IsDefault, 
                      dbo.Map.BeginX, dbo.Map.BeginY, dbo.Map.EndX, dbo.Map.EndY, dbo.CardLoc_2D.OccTime, dbo.CardLoc_2D.y AS y2, dbo.CardLoc_2D.CardNum, 
                      dbo.CardLoc_2D.CardType, dbo.CardLoc_2D.WorkSiteId, dbo.CardLoc_2D.x AS x2
FROM         dbo.CardLoc_2D INNER JOIN
                      dbo.WorkSite ON dbo.CardLoc_2D.WorkSiteId = dbo.WorkSite.WorkSiteId INNER JOIN
                      dbo.Map ON dbo.WorkSite.mapid = dbo.Map.Id

GO

CREATE VIEW [dbo].[cardLowPowerMsg_goods_Vw_2D]
AS
SELECT     dbo.CardLowPower.id, dbo.CardLowPower.CardNum, dbo.Articles.Tag AS Number, dbo.Articles.name, at.Name AS goodsType, dbo.Articles.departmentId, 
                      dbo.department.name AS department, dbo.CardLowPower.Power, dbo.CardLowPower.OccTime, dbo.CardLowPower.InDBTime, dbo.CardLowPower.IsRead, 
                      dbo.WorkSite.Address, dbo.Articles.RegisterTime
FROM         dbo.CardLowPower INNER JOIN
                      dbo.Articles ON dbo.CardLowPower.CardNum = dbo.Articles.cardNumber AND dbo.CardLowPower.CardType = 4 INNER JOIN
                      dbo.CardInformation AS ci ON ci.CardNum = dbo.CardLowPower.CardNum AND ci.CardType = dbo.CardLowPower.CardType LEFT OUTER JOIN
                      dbo.CardLoc_2D ON dbo.CardLoc_2D.CardNum = dbo.CardLowPower.CardNum AND dbo.CardLoc_2D.CardType = 4 LEFT OUTER JOIN
                      dbo.WorkSite ON dbo.WorkSite.WorkSiteId = dbo.CardLoc_2D.WorkSiteId LEFT OUTER JOIN
                      dbo.department ON dbo.Articles.departmentId = dbo.department.id LEFT OUTER JOIN
                      dbo.ArticlesType AS at ON at.id = dbo.Articles.type


GO


CREATE VIEW [dbo].[cardLowPowerMsg_Vw_2D]
AS
SELECT     dbo.CardLowPower.id, dbo.CardLowPower.CardNum, empvw.personName AS Name, empvw.PersonNo AS Number, empvw.departmentId, 
                      empvw.classTeamId, empvw.officePositionId, dbo.department.name AS department, dbo.classTeam.name AS classTeam, 
                      dbo.officePosition.name AS OfficePosition, dbo.CardLowPower.Power, dbo.CardLowPower.OccTime, dbo.CardLowPower.InDBTime, dbo.CardLowPower.IsRead, 
                      dbo.WorkSite.Address, empvw.inTime
FROM         dbo.CardLowPower left JOIN
                      dbo.AllPerson_view_2D as empvw ON dbo.CardLowPower.CardNum = empvw.cardNumber AND (dbo.CardLowPower.CardType = 3 or dbo.CardLowPower.CardType = 6 or dbo.CardLowPower.CardType = 9 or dbo.CardLowPower.CardType = 10) LEFT OUTER JOIN
                      dbo.CardLoc_2D ON dbo.CardLoc_2D.CardNum = dbo.CardLowPower.CardNum AND (dbo.CardLoc_2D.CardType = 3 or dbo.CardLoc_2D.CardType = 6 or dbo.CardLoc_2D.CardType = 3 or dbo.CardLoc_2D.CardType = 6) LEFT OUTER JOIN
                      dbo.WorkSite ON dbo.WorkSite.WorkSiteId = dbo.CardLoc_2D.WorkSiteId LEFT OUTER JOIN
                      dbo.department ON empvw.departmentId = dbo.department.id LEFT OUTER JOIN
                      dbo.classTeam ON empvw.classTeamId = dbo.classTeam.id AND dbo.department.id = dbo.classTeam.departId LEFT OUTER JOIN
                      dbo.officePosition ON dbo.officePosition.id = empvw.officePositionId




GO


CREATE view [dbo].[cardSosAlarmMsg_Vw_2D]
as
SELECT     dbo.CardSos.id, dbo.CardSos.CardNum,empvw.CarePersonName, dbo.CardSos.CardType,dbo.CardSos.WorkSiteId,empvw.personName,empvw.PersonNo,empvw.departmentId, empvw.classTeamId, 
                      empvw.officePositionId, 
                       dbo.CardSos.OccTime,dbo.CardSos.Lasttime,dbo.CardSos.IsRead,worksite.address,area.aName,empvw.inTime
FROM         dbo.CardSos INNER JOIN
              dbo.[OldPerson_view_2D] as empvw ON dbo.CardSos.CardNum = empvw.cardNumber and (CardSos.cardType=3 or CardSos.cardType=6) LEFT OUTER JOIN
			  dbo.worksite on dbo.worksite.worksiteid = dbo.cardSos.worksiteid LEFT OUTER JOIN
			  dbo.corAreaworksite  on dbo.CardSos.worksiteid=corAreaworksite.worksiteid LEFT OUTER JOIN
			  dbo.area on dbo.area.id=dbo.corAreaworksite.areaId where  (CardSos.cardType=3 or CardSos.cardType=6)





GO



CREATE VIEW [dbo].[NoSignal_Person_AlarmMsg_Vw_2D]
AS
SELECT     id, CardNum, personName,  departmentId,  classTeamId, officePositionId,  Address, OccTime, OkTime, IsRead
FROM         (SELECT     ce.id, ce.CardNum, p.personName, p.PersonNo, p.departmentId,  p.classTeamId, p.officePositionId, 
                                               ws.Address, ce.OccTime, ce.OkTime, ce.IsRead
                       FROM          dbo.CardException AS ce  INNER JOIN
                                              dbo.Person AS p ON p.cardNumber = ce.CardNum LEFT OUTER JOIN
                                              dbo.WorkSite AS ws ON ws.WorkSiteId = ce.WorkSiteId where 
                                               (ce.CardType = 3 or ce.CardType = 6 or ce.CardType=10)
                       UNION ALL
                       SELECT     ce.id, ce.CardNum, hp.personName, hp.personNo, hp.departmentId,  hp.classTeamId,  hp.officePositionId, 
                                             NULL AS Expr1, CONVERT(varchar(23), ce.OccTime, 120) AS occTime, CONVERT(varchar(23), ce.OkTime, 120) AS okTime, 
                                             ce.IsRead
                       FROM         dbo.CardException AS ce INNER JOIN
                                             dbo.HistoryPerson AS hp ON hp.cardNumber = ce.CardNum AND ( ce.CardType = 3  or  ce.CardType = 6 or ce.CardType=10) ) AS noSignalAlarm






GO



CREATE VIEW [dbo].[PersonAndArticleCardLowPowerMsg_View_2D]
AS
SELECT     id, CardNum, Name, Number, InDBTime, OccTime, IsRead
FROM         dbo.cardLowPowerMsg_Vw_2D
UNION ALL
SELECT     id, CardNum, Name, Number, InDBTime, OccTime, IsRead
FROM         dbo.cardLowPowerMsg_goods_Vw_2D

GO

CREATE VIEW [dbo].[PersonAndArticlesName_View_2D]
AS
SELECT     id, cardNumber, personName, PersonNo as number
FROM         dbo.Person
UNION ALL
SELECT     id, cardNumber, name, Tag AS Number
FROM         dbo.Articles

GO



CREATE VIEW [dbo].[PersonCardLoc_View_2D]
AS
SELECT     dbo.CardLoc_2D.CardNum, dbo.CardLoc_2D.CardType, dbo.CardLoc_2D.x, dbo.CardLoc_2D.y, dbo.CardLoc_2D.OccTime, empvw.id, empvw.cardNumber, 
                      empvw.personName, empvw.number, empvw.officePositionId, empvw.classTeamId, empvw.departmentId, empvw.sex, empvw.mobile, 
                      empvw.telephone, empvw.identifyNum, empvw.photoExtends, empvw.photo, empvw.inTime, empvw.outTime, dbo.officePosition.name, 
                      dbo.officePosition.position, dbo.officePosition.isCadres, dbo.officePosition.[level], dbo.department.name AS DepartmentName, dbo.WorkSite.mapid, 
                      dbo.WorkSite.Address, dbo.WorkSite.Type AS WorkSiteType, dbo.Map.ParentId, dbo.Map.Path, dbo.Map.Name AS MapName
FROM         dbo.CardLoc_2D INNER JOIN
                      dbo.employeeview as empvw ON dbo.CardLoc_2D.CardNum = empvw.cardNumber INNER JOIN
                      dbo.officePosition ON empvw.officePositionId = dbo.officePosition.id INNER JOIN
                      dbo.department ON empvw.departmentId = dbo.department.id INNER JOIN
                      dbo.WorkSite ON dbo.CardLoc_2D.WorkSiteId = dbo.WorkSite.WorkSiteId INNER JOIN
                      dbo.Map ON dbo.WorkSite.mapid = dbo.Map.Id
WHERE     (dbo.CardLoc_2D.CardType = 3 or dbo.CardLoc_2D.CardType = 6 )

GO



CREATE VIEW [dbo].[PersonHistroyPath_View_2D]
AS
SELECT     dbo.CardLoc_Interval_2D.id, dbo.CardLoc_Interval_2D.CardNum, dbo.CardLoc_Interval_2D.CardType, dbo.CardLoc_Interval_2D.WorkSiteId, 
                      dbo.CardLoc_Interval_2D.x, dbo.CardLoc_Interval_2D.y, dbo.CardLoc_Interval_2D.OccTime, dbo.CardLoc_Interval_2D.IsOnWorksite, 
                      dbo.CardLoc_Interval_2D.Objectid, empvw.personName AS Name, empvw.number AS No, dbo.WorkSite.mapid, dbo.Map.ParentId, 
                      dbo.Map.Name AS MapName, dbo.Map.Path
FROM         dbo.CardLoc_Interval_2D INNER JOIN
                      dbo.EmployeeView as empvw ON dbo.CardLoc_Interval_2D.Objectid = empvw.id INNER JOIN
                      dbo.WorkSite ON dbo.CardLoc_Interval_2D.WorkSiteId = dbo.WorkSite.WorkSiteId INNER JOIN
                      dbo.Map ON dbo.WorkSite.mapid = dbo.Map.Id
WHERE     (dbo.CardLoc_Interval_2D.CardType = 3) OR
                      (dbo.CardLoc_Interval_2D.CardType = 6)
                      or (dbo.CardLoc_Interval_2D.CardType = 10)
GO


CREATE VIEW [dbo].[OldPersonHistroyPath_View_2D]
AS
SELECT     dbo.CardLoc_Interval_2D.id, dbo.CardLoc_Interval_2D.CardNum, dbo.CardLoc_Interval_2D.CardType, dbo.CardLoc_Interval_2D.WorkSiteId, 
                      dbo.CardLoc_Interval_2D.x, dbo.CardLoc_Interval_2D.y, dbo.CardLoc_Interval_2D.OccTime, dbo.CardLoc_Interval_2D.IsOnWorksite, 
                      dbo.CardLoc_Interval_2D.Objectid, empvw.personName AS Name, empvw.PersonName AS No, dbo.WorkSite.mapid, dbo.Map.ParentId, 
                      dbo.Map.Name AS MapName, dbo.Map.Path
FROM         dbo.CardLoc_Interval_2D INNER JOIN
                      dbo.OldPerson_view_2D as empvw ON dbo.CardLoc_Interval_2D.Objectid = empvw.id INNER JOIN
                      dbo.WorkSite ON dbo.CardLoc_Interval_2D.WorkSiteId = dbo.WorkSite.WorkSiteId INNER JOIN
                      dbo.Map ON dbo.WorkSite.mapid = dbo.Map.Id
WHERE     (dbo.CardLoc_Interval_2D.CardType = 3) OR
                      (dbo.CardLoc_Interval_2D.CardType = 6) or
                       (dbo.CardLoc_Interval_2D.CardType=10)

GO



CREATE VIEW [dbo].[FaultMattressAlarm_View_2D]
AS
SELECT     dbo.CardException.CardNum  as WorkSiteId, dbo.CardException.OccTime, 
dbo.CardException.OkTime, dbo.CardException.IsRead, 
Area.aName+dbo.NurseUnit.Address as Address, dbo.CardException.id,
    cm.CarePersonName
FROM         dbo.CardException left join
             NurseUnit on CardException.CardNum=NurseUnit.MattressNum left join ( select MAX(PersonId) as PersonId,NurseUnitId from Customer group by NurseUnitId) as cu on
             
             cu.NurseUnitId=NurseUnit.id  left join Area on Area.id=NurseUnit.AreaId
             
             left join OldPerson_view_2D as cm on cm.Id=cu.PersonId 
                    

  where   CardException.CardType=8

GO


create view  PagerUserNameCardnumber_Vw_2D

as
SELECT  distinct(PagerNum),STUFF((SELECT ','+[PersonName] 
 FROM (   select PagerNum,PersonName,CardNumber from NurseUnit left 
 join Customer on Customer.NurseUnitId=NurseUnit.id
 left join Person on Person.id=Customer.PersonId) b WHERE PagerNum = A.PagerNum
 FOR XML PATH('')) ,1,1,'')AS PersonName,
 STUFF((SELECT ','+[CardNumber] 
 FROM (   select PagerNum,PersonName,CardNumber from NurseUnit left 
 join Customer on Customer.NurseUnitId=NurseUnit.id
 left join Person on Person.id=Customer.PersonId) b WHERE PagerNum = A.PagerNum
 FOR XML PATH('')) ,1,1,'')AS CardNumber
 FROM (   select PagerNum,PersonName,CardNumber from NurseUnit left 
 join Customer on Customer.NurseUnitId=NurseUnit.id
 left join Person on Person.id=Customer.PersonId) AS A  
 where PagerNum>0
 
 
 go

  
  create view PagerAlarmAddress_Vw_2D
  as
   
   SELECT  distinct(PagerNum),STUFF((SELECT ','+[Address] 
 FROM (select   aName+Address as Address,NurseUnit.id,
 PagerNum from NurseUnit left join Area on NurseUnit.AreaId=Area.id) b WHERE PagerNum = A.PagerNum
 FOR XML PATH('')) ,1,1,'')AS Address
 FROM (select   aName+Address as Address,NurseUnit.id,
 PagerNum from NurseUnit left join Area on NurseUnit.AreaId=Area.id) AS A  
 where PagerNum>0
 
 go
 
 
 create view CarePersonName_Vw_2D
 as
 
 
 SELECT  distinct(PagerNum),STUFF((SELECT ','+[CarePersonName] 
 FROM (  select PagerNum,PersonName as CarePersonName,CardNumber as CareCardNumber
   from NurseUnit left join Customer on NurseUnit.id=Customer.NurseUnitId
   left join dbo.CarePersonName_view  cp on cp.CustomerId=Customer.PersonId 
   group by PagerNum,PersonName,CardNumber) b WHERE PagerNum = A.PagerNum
 FOR XML PATH('')) ,1,1,'')AS CarePersonName
 FROM (  select PagerNum,PersonName as CarePersonName,CardNumber as CareCardNumber
   from NurseUnit left join Customer on NurseUnit.id=Customer.NurseUnitId
   left join dbo.CarePersonName_view  cp on cp.CustomerId=Customer.PersonId 
   group by PagerNum,PersonName,CardNumber) AS A  
 where PagerNum>0
  
  go


CREATE VIEW [dbo].[FaultPagerAlarm_View_2D]
AS
SELECT     dbo.CardException.CardNum  as WorkSiteId, dbo.CardException.OccTime,
 dbo.CardException.OkTime, dbo.CardException.IsRead,PagerAlarmAddress_Vw_2D.Address,  dbo.CardException.id,CarePersonName_Vw_2D.CarePersonname as CarePersonName
FROM  dbo.CardException  left join
 PagerAlarmAddress_Vw_2D on PagerAlarmAddress_Vw_2D.PagerNum=CardException.CardNum left join
CarePersonName_Vw_2D   on  CarePersonName_Vw_2D.PagerNum=CardException.CardNum

  where   CardException.CardType=9  





GO

CREATE VIEW [dbo].[WorkSiteAlarm_View_2D]
AS
SELECT     dbo.WorkSiteAlarm.WorkSiteId, dbo.WorkSiteAlarm.OccTime, dbo.WorkSiteAlarm.OkTime, dbo.WorkSiteAlarm.IsRead, dbo.WorkSite.Address, dbo.WorkSiteAlarm.id, 
                      dbo.Area.aName as Name
FROM         dbo.WorkSiteAlarm LEFT JOIN
                      dbo.WorkSite ON dbo.WorkSiteAlarm.WorkSiteId = dbo.WorkSite.WorkSiteId LEFT JOIN
                      dbo.corAreaWorksite ON dbo.WorkSite.WorkSiteId = dbo.corAreaWorksite.WorksiteId LEFT JOIN
                      dbo.Area ON dbo.Area.id = dbo.corAreaWorksite.areaId
WHERE     (dbo.WorkSiteAlarm.WorkSiteId >= 1000) AND (dbo.WorkSiteAlarm.WorkSiteId < 65535)


GO

CREATE VIEW [dbo].[WorkSiteMapView_2D]
AS
SELECT     dbo.WorkSite.WorkSiteId, dbo.WorkSite.Address, dbo.WorkSite.Type, dbo.WorkSite.mapid, dbo.Map.Id, dbo.Map.Name, dbo.Map.Path, dbo.Map.ParentId, 
                      dbo.Map.MapOrder, dbo.Map.IsDefault, dbo.Map.BeginX, dbo.Map.BeginY, dbo.Map.EndX, dbo.Map.EndY, dbo.WorkSite.y2, dbo.WorkSite.x2
FROM         dbo.WorkSite INNER JOIN
                      dbo.Map ON dbo.WorkSite.mapid = dbo.Map.Id

GO




CREATE VIEW [dbo].[WorkSiteUnitAlarm_View_2D]
AS
SELECT     dbo.WorkSiteAlarm.WorkSiteId, dbo.WorkSiteAlarm.OccTime, dbo.WorkSiteAlarm.OkTime, dbo.WorkSiteAlarm.IsRead, dbo.WorkSite.Address, dbo.WorkSiteAlarm.id
FROM         dbo.WorkSiteAlarm LEFT JOIN
                      dbo.WorkSite ON dbo.WorkSiteAlarm.WorkSiteId = dbo.WorkSite.WorkSiteId
WHERE     (dbo.WorkSiteAlarm.WorkSiteId < 1000)


GO


CREATE VIEW [dbo].[EvacuateFeedBackView]
AS
 select ef.*,empvw.personName,empvw.number,d.name as DepartmentName,o.name as OfficePositionName from EvacuateFeedback ef 
 left join EmployeeView empvw on empvw.cardNumber = ef.CardNumber
 left join department d on d.id = empvw.departmentId
 left join officePosition o on o.id = empvw.officePositionId
GO

CREATE VIEW [dbo].[PersonAreaPassView_2D]
AS
SELECT     dbo.AreaPass_2D.Id, dbo.AreaPass_2D.CardNumber, dbo.AreaPass_2D.AreaName, dbo.AreaPass_2D.ObjectId, dbo.AreaPass_2D.MapId, dbo.AreaPass_2D.OccTime, 
                      dbo.AreaPass_2D.X, dbo.AreaPass_2D.Y, empvw.personName AS name, dbo.CardLoc_2D.CardType, empvw.number AS Number
FROM         dbo.AreaPass_2D LEFT OUTER JOIN
                      dbo.EmployeeView as empvw ON dbo.AreaPass_2D.ObjectId = empvw.id LEFT OUTER JOIN
                      dbo.CardLoc_2D ON dbo.AreaPass_2D.CardNumber = dbo.CardLoc_2D.CardNum
WHERE     (dbo.CardLoc_2D.CardType = 3 or dbo.CardLoc_2D.CardType = 6)
GO

CREATE VIEW [dbo].[ArticleAreaPassView_2D]
AS
SELECT     dbo.AreaPass_2D.Id, dbo.AreaPass_2D.CardNumber, dbo.AreaPass_2D.AreaName, dbo.AreaPass_2D.ObjectId, dbo.AreaPass_2D.MapId, dbo.AreaPass_2D.OccTime, 
                      dbo.AreaPass_2D.X, dbo.AreaPass_2D.Y, dbo.Articles.name, dbo.CardLoc_2D.CardType, dbo.Articles.Tag AS Number
FROM         dbo.AreaPass_2D LEFT OUTER JOIN
                      dbo.Articles ON dbo.AreaPass_2D.ObjectId = dbo.Articles.id LEFT OUTER JOIN
                      dbo.CardLoc_2D ON dbo.AreaPass_2D.CardNumber = dbo.CardLoc_2D.CardNum
WHERE     (dbo.CardLoc_2D.CardType = 4)
GO


CREATE VIEW [dbo].[WristFallAlarmView_2D]
AS
SELECT     empvw.personName, empvw.PersonNo, empvw.officePositionId, empvw.classTeamId, empvw.departmentId, empvw.sex,
 dbo.WristFallAlarm.id, empvw.CarePersonName,
                      dbo.WristFallAlarm.CardNum, dbo.WristFallAlarm.CardType, dbo.WristFallAlarm.WorkSiteId, dbo.WristFallAlarm.OccTime, dbo.WristFallAlarm.IsRead, 
                      dbo.WorkSite.Address, dbo.WorkSite.mapid
FROM         dbo.WristFallAlarm INNER JOIN
                      dbo.OldPerson_view_2D as empvw ON dbo.WristFallAlarm.CardNum = empvw.cardNumber LEFT OUTER JOIN
                      dbo.WorkSite ON dbo.WristFallAlarm.WorkSiteId = dbo.WorkSite.WorkSiteId 
                      where WristFallAlarm.cardType=6
                      
GO



 CREATE view [dbo].[PagerLowerAlarm_View_2D]
 
 as
 
 select CardLowPower.IsRead,CardLowPower.InDBTime,CardLowPower.OccTime,CardLowPower.CardType,'' as Number,
 CardLowPower.Id,CardLowPower.CardNum as CardNum,Address as Name from
 CardLowPower left join  PagerAlarmAddress_Vw_2D on CardLowPower.CardNum=PagerAlarmAddress_Vw_2D.PagerNum
 where CardLowPower.CardType='9'

GO


create view [dbo].[ChestCardLowAlarmMsg_View_2D]

as 

select CardLowPower.id as Id,CardLowPower.CardNum,Person.PersonName as Name,
 CardLowPower.CardType,Person.PersonNo as Number,CardLowPower.InDBTime,CardLowPower.OccTime,
 CardLowPower.IsRead 
 from CardLowPower left join Person on CardLowPower.CardNum=Person.CardNumber
 where CardLowPower.CardType=10 
GO

 create view [dbo].[DownMessage_View]
 as
  select DownMessage.id,DownMessage.Type,DownMessage.CardList,DownMessage.Content,
  DownMessage.OccTime,DownMessage.SiteList,DownMessage.Status,Person.PersonName
  from DownMessage left join Person on DownMessage.CardList=Person.CardNumber
GO

create view [dbo].[AutoCardLowAlarmMsg_View_2D]

as 

select CardLowPower.id as Id,CardLowPower.CardNum,Person.PersonName as Name,
 CardLowPower.CardType,Person.PersonNo as Number,CardLowPower.InDBTime,CardLowPower.OccTime,
 CardLowPower.IsRead 
 from CardLowPower left join Person on CardLowPower.CardNum=Person.CardNumber
 where CardLowPower.CardType=3 or CardLowPower.CardType=4
GO


create view [dbo].[WristCardLowAlarmMsg_View_2D]

as 

select CardLowPower.id as Id,CardLowPower.CardNum,Person.PersonName as Name,
 CardLowPower.CardType,Person.PersonNo as Number,CardLowPower.InDBTime,CardLowPower.OccTime,
 CardLowPower.IsRead 
 from CardLowPower left join Person on CardLowPower.CardNum=Person.CardNumber
 where CardLowPower.CardType=6
GO


CREATE VIEW [dbo].[WristCardSilentAlarmView_2D]
AS
SELECT     dbo.WristCardSilentAlarm.id, dbo.WristCardSilentAlarm.CardNum, dbo.WristCardSilentAlarm.CardType, dbo.WristCardSilentAlarm.OccTime, 
                      dbo.WristCardSilentAlarm.OkTime, dbo.WristCardSilentAlarm.IsRead, dbo.WristCardSilentAlarm.Msg,  
                      empvw.personName, empvw.PersonNo, empvw.officePositionId, empvw.departmentId, empvw.sex,empvw.CarePersonName,
                      dbo.WristCardSilentAlarm.WorkSiteId, (dbo.WorkSite.Address) as Address
FROM         dbo.WristCardSilentAlarm INNER JOIN
                     dbo.OldPerson_view_2D as empvw ON dbo.WristCardSilentAlarm.CardNum = empvw.cardNumber LEFT OUTER JOIN
                      dbo.WorkSite ON dbo.WristCardSilentAlarm.WorkSiteId = dbo.WorkSite.WorkSiteId LEFT OUTER JOIN
                       Customer on Customer.PersonId=empvw.Id left outer join NurseUnit on Customer.NurseUnitId=NurseUnit.id
                    

GO

CREATE VIEW [dbo].[DisassemblyAlarmView_2D]
AS
SELECT     dbo.WristDisConAlarm.id, dbo.WristDisConAlarm.CardNum, dbo.WristDisConAlarm.CardType, dbo.WristDisConAlarm.OccTime, dbo.WristDisConAlarm.IsRead, 
                 empvw.personName, empvw.PersonNo,empvw.CarePersonName, WorkSite.Address,
                      empvw.sex, empvw.departmentId, empvw.officePositionId, dbo.WristDisConAlarm.OkTime
FROM         dbo.WristDisConAlarm INNER JOIN
                      dbo.OldPerson_view_2D as empvw ON dbo.WristDisConAlarm.CardNum = empvw.cardNumber LEFT OUTER JOIN
                   Customer on Customer.PersonId=empvw.Id left outer join   dbo.WorkSite ON dbo.WristDisConAlarm.WorkSiteId = dbo.WorkSite.WorkSiteId 
                      where WristDisConAlarm.CardType=6



GO

CREATE view [dbo].[pagerCallAlarmMsg_View_2D]
as

select puc.PersonName,
PagerCallAlarm.IsRead,PagerCallAlarm.PagerNum,PagerCallAlarm.OccTime,
 pa.Address,a.CarePersonName as CustomerId
 ,PagerCallAlarm.id,puc.CardNumber
from PagerCallAlarm left join PagerUserNameCardnumber_Vw_2D  as puc on 
PagerCallAlarm.PagerNum=puc.PagerNum
left join  PagerAlarmAddress_Vw_2D as pa  on   pa.PagerNum=PagerCallAlarm.PagerNum 
 left join  CarePersonName_Vw_2D as a on   a.PagerNum=PagerCallAlarm.PagerNum



GO


CREATE view [dbo].[MattressAlarmMsg_View_2D]
as
select Person.PersonName,MattressStatusAlarm.IsRead,
MattressStatusAlarm.MattressId,MattressStatusAlarm.StartTime,MattressStatusAlarm.Status,
MattressStatusAlarm.OkTime,
 Area.aName+ NurseUnit.Address as Address,a.CarePersonName as CustomerId,MattressStatusAlarm.id,Person.CardNumber
from MattressStatusAlarm left join NurseUnit on 
MattressStatusAlarm.MattressId=NurseUnit.MattressNum
left join Area on NurseUnit.AreaId=Area.id
left join (select MAX(PersonId) as PersonId,NurseUnitId from Customer group by NurseUnitId) as cu  on cu.NurseUnitId=NurseUnit.id left join Person on
Person.id=cu.PersonId left join (SELECT  DISTINCT [customerid],STUFF((SELECT ','+[personname]
 FROM (select customerid,personname,PersonNo,CardNumber from CarePersonName_view) b WHERE customerid = A.customerid
 FOR XML PATH('')) ,1,1,'')AS CarePersonName
 FROM (select customerid,personname,PersonNo,CardNumber from CarePersonName_view) AS A) 
 a on a.CustomerId=Person.id


GO


create view [dbo].[MattressAlarmSet_Vw_2D]

as

SELECT  STUFF((SELECT ','+[timeSpan]
 FROM (  select CONVERT(nvarchar,DATEPART(hh,StartTime))+':'+
 (case when len(CONVERT(nvarchar,DATEPART(MI,StartTime)))>1 then CONVERT(nvarchar,DATEPART(MI,StartTime))
  when CONVERT(nvarchar,DATEPART(MI,StartTime))<=1 then CONVERT(nvarchar,DATEPART(MI,StartTime))+'0' end)
  +'-'+CONVERT(nvarchar,DATEPART(hh,EndTime))+':'+
 (case when len(CONVERT(nvarchar,DATEPART(MI,EndTime)))>1 then CONVERT(nvarchar,DATEPART(MI,EndTime))
  when CONVERT(nvarchar,DATEPART(MI,EndTime))<=1 then CONVERT(nvarchar,DATEPART(MI,EndTime))+'0' end)
 as TimeSpan,ROW_NUMBER() over(order by endtime asc) newno  from  MattressAlarmSet) b  order by newno asc
 FOR XML PATH('')) ,1,1,'') as Value,'OutBedAlarmSpan' as Item,
 '离床报警时间段,逗号隔开' as Description
GO

------------------------------------------------------View End ------------------------------------------

-----------------------------------------------------------Procedure -------------------------------------


create procedure dbo.SP_AddFloder (
     @filepath   VARCHAR(255)  OUT
   ) as
BEGIN

     DECLARE @sqlTxt VARCHAR(500)

     SET @filepath=NULL

     SELECT @filepath=Value FROM Settings WHERE Item='FileGroupPath'

     IF @filepath IS NOT NULL

     BEGIN

       DECLARE @temp  TABLE(a  int,b   int,c   int)   

       INSERT   @temp   EXEC   master..xp_fileexist   @filepath   

       IF  NOT EXISTS(select * from @temp   where   b=1)

       BEGIN  

         EXEC sp_configure 'show advanced options',1

         RECONFIGURE

         EXEC sp_configure 'xp_cmdshell',1

         RECONFIGURE

         SET @sqlTxt='mkdir '+@filepath

         EXEC xp_cmdshell @sqlTxt

         RECONFIGURE

         EXEC sp_configure 'xp_cmdshell',0

         RECONFIGURE

         EXEC sp_configure 'show advanced options',0

         RECONFIGURE

       END

   END

   ELSE

   BEGIN

       SELECT @filepath=RTRIM(REVERSE(filename)) from sysfiles   

       SELECT @filepath=REVERSE(SUBSTRING(@filepath,CHARINDEX('/',@filepath),8000)) 

       SELECT @filepath=SUBSTRING(@filepath,1,LEN(@filepath)-CHARINDEX('\',REVERSE(@filepath))+1) 

    END

  IF SUBSTRING(REVERSE(@filepath),1,1)<>'\' 

   BEGIN

    SET @filepath=@filepath+'\'

   END

 END
------------------------------------------------------------------------
go


create procedure dbo.SP_CheckIndex (
 @tableName VARCHAR(100),
 @indexname varchar(100) OUT,
 @indexColName varchar(100) OUT,
 @isPK varchar(10) OUT --主键是否是聚集索引
 
 ) as
BEGIN

   set nocount on

 

   declare @objid int,      -- the object id of the table

        @indid smallint, -- the index id of an index

        @groupid int,       -- the filegroup id of an index

        @indname sysname,

        @groupname sysname,

        @status int,

        @keys nvarchar(2126), --Length (16*max_identifierLength)+(15*2)+(16*3)

        @dbname sysname,

        @ignore_dup_key  bit,

        @is_unique    bit,

        @is_hypothetical bit,

        @is_primary_key  bit,

        @is_unique_key   bit,

        @auto_created bit,

        @no_recompute bit

            

   -- Check to see that the object names are local to the current database.

   select @dbname = parsename(@tableName,3)

   if @dbname is null

      select @dbname = db_name()

   else if @dbname <> db_name()

      begin

        raiserror(15250,-1,-1)

          SET @indexname=NULL

            SET @indexColName=NULL

            SET @isPK=0

        return (1)

      end

 

   -- Check to see the the table exists and initialize @objid.

   select @objid = object_id(@tableName)

   if @objid is NULL

   begin

      raiserror(15009,-1,-1,@tableName,@dbname)

      SET @indexname=NULL

        SET @indexColName=NULL

        SET @isPK=0

      return (1)

   end

 

   -- OPEN CURSOR OVER INDEXES (skip stats: bug shiloh_51196)

   declare ms_crs_ind cursor local static for

      select i.index_id, i.data_space_id, i.name,

        i.ignore_dup_key, i.is_unique, i.is_hypothetical, i.is_primary_key, i.is_unique_constraint,

        s.auto_created, s.no_recompute

      from sys.indexes i join sys.stats s

        on i.object_id = s.object_id and i.index_id = s.stats_id

      where i.object_id = @objid

   open ms_crs_ind

   fetch ms_crs_ind into @indid, @groupid, @indname, @ignore_dup_key, @is_unique, @is_hypothetical,

        @is_primary_key, @is_unique_key, @auto_created, @no_recompute

 

   -- IF NO INDEX, QUIT

   if @@fetch_status < 0

   begin

      deallocate ms_crs_ind

      raiserror(15472,-1,-1,@tableName) -- Object does not have any indexes.

      SET @indexname=NULL

        SET @indexColName=NULL

        SET @isPK=0

      return (0)

   end

 

   -- create temp table

   CREATE TABLE #spindtab

   (

      index_name       sysname collate database_default NOT NULL,

      index_id         int,

      ignore_dup_key      bit,

      is_unique           bit,

      is_hypothetical     bit,

      is_primary_key      bit,

      is_unique_key       bit,

      auto_created        bit,

      no_recompute        bit,

      groupname        sysname collate database_default NULL,

      index_keys       nvarchar(2126)   collate database_default NOT NULL -- see @keys above for length descr

   )

 

   -- Now check out each index, figure out its type and keys and

   -- save the info in a temporary table that we'll print out at the end.

   while @@fetch_status >= 0

   begin

      -- First we'll figure out what the keys are.

      declare @i int, @thiskey nvarchar(131) -- 128+3

 

      select @keys = index_col(@tableName, @indid, 1), @i = 2

      if (indexkey_property(@objid, @indid, 1, 'isdescending') = 1)

        select @keys = @keys  + '(-)'

 

      select @thiskey = index_col(@tableName, @indid, @i)

      if ((@thiskey is not null) and (indexkey_property(@objid, @indid, @i, 'isdescending') = 1))

        select @thiskey = @thiskey + '(-)'

 

      while (@thiskey is not null )

      begin

        select @keys = @keys + ', ' + @thiskey, @i = @i + 1

        select @thiskey = index_col(@tableName, @indid, @i)

        if ((@thiskey is not null) and (indexkey_property(@objid, @indid, @i, 'isdescending') = 1))

           select @thiskey = @thiskey + '(-)'

      end

 

      select @groupname = null

      select @groupname = name from sys.data_spaces where data_space_id = @groupid

 

      -- INSERT ROW FOR INDEX

      insert into #spindtab values (@indname, @indid, @ignore_dup_key, @is_unique, @is_hypothetical,

        @is_primary_key, @is_unique_key, @auto_created, @no_recompute, @groupname, @keys)

 

      -- Next index

      fetch ms_crs_ind into @indid, @groupid, @indname, @ignore_dup_key, @is_unique, @is_hypothetical,

        @is_primary_key, @is_unique_key, @auto_created, @no_recompute

   end

   deallocate ms_crs_ind

 

   -- DISPLAY THE RESULTS

   select

      'index_name' = index_name,

      'index_description' = convert(varchar(210), --bits 16 off, 1, 2, 16777216 on, located on group

           case when index_id = 1 then 'clustered' else 'nonclustered' end

           + case when ignore_dup_key <>0 then ', ignore duplicate keys' else '' end

           + case when is_unique <>0 then ', unique' else '' end

           + case when is_hypothetical <>0 then ', hypothetical' else '' end

           + case when is_primary_key <>0 then ', primary key' else '' end

           + case when is_unique_key <>0 then ', unique key' else '' end

           + case when auto_created <>0 then ', auto create' else '' end

           + case when no_recompute <>0 then ', stats no recompute' else '' end

           + ' located on ' + groupname),

      'index_keys' = index_keys INTO #indexTable

   from #spindtab 

   order by index_name

   ----------------------------------------------------------

 

SET @indexname=NULL

SET @indexColName=NULL

SELECT @indexname=index_name,@indexColName=index_keys FROM #indexTable WHERE index_description NOT LIKE '%nonclustered%' AND index_description LIKE '%primary key%'

IF @indexname IS NOT NULL AND @indexColName IS NOT NULL

BEGIN

SET @isPK='1'

END

ELSE

BEGIN

SELECT @indexname=index_name,@indexColName=index_keys FROM #indexTable WHERE index_description NOT LIKE '%nonclustered%'

IF @indexname IS NOT NULL AND @indexColName IS NOT NULL

BEGIN

SET @isPK='0'

END

END

END
go


create procedure dbo.SP_AddFileGroup @tableName VARCHAR(255),
 
 @tableColunm VARCHAR(100) as
BEGIN

 DECLARE @PKColName VARCHAR(100)

 DECLARE @PKName VARCHAR(100)

 DECLARE @dbGroupName VARCHAR(255)

 DECLARE @fileGroupName VARCHAR(255)

 DECLARE @filepath varchar(255) 

 DECLARE @dbName VARCHAR(255)

 DECLARE @strSql VARCHAR(MAX)  

 DECLARE @fileYear VARCHAR(4)

 DECLARE @fileSize VARCHAR(255)

 DECLARE @fizeAddSize VARCHAR(255)

 DECLARE @PFName VARCHAR(255)

 DECLARE @PSName VARCHAR(255)

 DECLARE @IsPK VARCHAR(10)

 DECLARE @PKName1 VARCHAR(100)

 DECLARE @PKColName1 VARCHAR(100)

 DECLARE @IsPK1 varchar(100)

 DECLARE @PSGroupName VARCHAR(MAX)

 DECLARE @i INT

 DECLARE @j INT

 DECLARE @curr INT 

 DECLARE @prev INT

 EXEC SP_AddFloder  @filepath  OUT

 SELECT @dbName=DB_NAME()

 SELECT @fileYear=RTRIM(LTRIM(STR(YEAR(GETDATE()))))

 SET @fileSize='100MB' --单位MB 兆

 SET @fizeAddSize='5MB' --单位MB 兆

 SET @PSGroupName=''

 SET @i=1

 SET @curr=1 

 SET @prev=1

 WHILE @i<5

 BEGIN

  SET @dbGroupName=@tableName+'_'+@fileYear+RTRIM(LTRIM(STR(@i)))
    --判断是否存在该文件组,存在则跳出存储过程
    IF NOT EXISTS (SELECT 1 FROM sys.filegroups WHERE UPPER(name)=UPPER(@dbGroupName))
    BEGIN
	 SET @strSql='ALTER DATABASE '+@dbName+' ADD FILEGROUP '+@dbGroupName
	 PRINT @strSql
	 EXEC(@strSql)
	 SET @strSql='ALTER DATABASE ' +@dbName+' ADD FILE (NAME ='''+@dbGroupName+''',FILENAME='''+@filepath+@dbGroupName+'.ndf'',SIZE='+@fileSize+',FILEGROWTH='+@fizeAddSize+') TO FILEGROUP '+@dbGroupName
		 PRINT @strSql
		 EXEC(@strSql)
     END
     ELSE
     BEGIN
	 RETURN 
     END

 SET @PSGroupName=@PSGroupName+@dbGroupName+','
 SET @i=@i+1

 END

 SET @PSGroupName=SUBSTRING(@PSGroupName,1,LEN(@PSGroupName)-1)

 SET @i=1

 SET @curr=1 

 SET @prev=1

 SET @PFName='PF_'+@tableName

 IF  EXISTS (SELECT * FROM sys.partition_functions

    WHERE UPPER(name) = UPPER(@PFName))

      BEGIN

      SET @PSName='PS_'+@tableName

 IF  EXISTS (SELECT * FROM sys.partition_schemes

    WHERE UPPER(name) = UPPER(@PSName))

      BEGIN

           WHILE @prev < LEN(@PSGroupName) 

          BEGIN 

          SET @curr=CHARINDEX(',',@PSGroupName,@prev) 

             IF @curr>@prev 

                 BEGIN

                   SET @strSql='ALTER PARTITION SCHEME '+@PSName+' NEXT USED '+SUBSTRING(@PSGroupName,@prev,@curr-@prev)

                   PRINT @strSql

                   EXEC(@strSql)

                   IF(@i=1)

                   BEGIN

                      SET @strSql='ALTER PARTITION FUNCTION '+@PFName+'() SPLIT RANGE ('''+@fileYear+'0401'')'

                      PRINT @strSql

                      EXEC(@strSql)

                   END

                   ELSE IF(@i=2)

                   BEGIN

                       SET @strSql='ALTER PARTITION FUNCTION '+@PFName+'() SPLIT RANGE ('''+@fileYear+'0701'')'

                             PRINT @strSql

                             EXEC(@strSql)

                   END

                   ELSE IF(@i=3) 

                   BEGIN

                       SET @strSql='ALTER PARTITION FUNCTION '+@PFName+'() SPLIT RANGE ('''+@fileYear+'1001'')'

                             PRINT @strSql

                             EXEC(@strSql)

                   END

                   SET @i=@i+1

                END

             ELSE 

                BEGIN 

                   SET @strSql='ALTER PARTITION SCHEME '+@PSName+' NEXT USED '+SUBSTRING(@PSGroupName,@prev,LEN(@PSGroupName)-@prev+1) 

                   PRINT @strSql

                   EXEC(@strSql)

                   BREAK

                END

          SET @prev=@curr+1 

           END

      END

    ELSE

      BEGIN

         SET @strSql='CREATE PARTITION SCHEME '+@PSName+' AS PARTITION '+@PFName +' TO('+@PSGroupName+')'

           PRINT @strSql

           EXEC(@strSql)

      END

      END

    ELSE

      BEGIN

         SET @strSql='CREATE PARTITION FUNCTION '+@PFName+'(DATETIME) AS RANGE RIGHT FOR VALUES ('''+@fileYear+'0401'','+''''+@fileYear+'0701'','+''''+@fileYear+'1001'')'

           PRINT @strSql

           EXEC(@strSql) 

      END

 SET @i=1

 SET @curr=1 

 SET @prev=1 

 SET @PSName='PS_'+@tableName

 IF NOT EXISTS (SELECT * FROM sys.partition_schemes

    WHERE UPPER(name) = UPPER(@PSName))

      BEGIN

         SET @strSql='CREATE PARTITION SCHEME '+@PSName+' AS PARTITION '+@PFName +' TO('+@PSGroupName+')'

           PRINT @strSql

           EXEC(@strSql)

      END

SET @PKName=NULL

SET @PKColName=NULL 

EXEC SP_CheckIndex @tableName,@PKName OUT ,@PKColName OUT,@IsPK OUT

IF @PKName IS NOT NULL AND @PKColName IS NOT NULL AND @IsPK='1'

BEGIN

   SET @strSql='ALTER TABLE '+ @tableName +' DROP CONSTRAINT '+@PKName

   PRINT @strSql

   EXEC(@strSql)

   SET @PKName1=NULL

   SET @PKColName1=NULL

   EXEC SP_CheckIndex @tableName,@PKName1 OUT ,@PKColName1,@IsPK1 OUT

   IF @PKName1 IS NOT NULL AND @PKColName1 IS NOT NULL

   BEGIN

      SET @strSql='DROP INDEX ' +@PKName+' ON '+@tableName

      PRINT @strSql

      EXEC(@strSql)

   END

   SET @strSql='ALTER TABLE ' + @tableName+ ' ADD CONSTRAINT '+@PKName +' PRIMARY KEY NONCLUSTERED ('+@PKColName+' ASC) ON [PRIMARY]'

   PRINT @strSql

   EXEC(@strSql)

   SET @strSql='CREATE CLUSTERED INDEX CT_'+@tableName +'_'+@tableColunm+' ON '+@tableName+'('+@tableColunm+') ON '+@PSName+'('+@tableColunm+')'

   PRINT @strSql

   EXEC(@strSql)

   END

ELSE IF @PKName IS NOT NULL AND @PKColName IS NOT NULL AND @IsPK='0'

BEGIN

   SET @strSql='DROP INDEX ' +@PKName+' ON '+@tableName

    PRINT @strSql

    EXEC(@strSql)

   SET @strSql='CREATE CLUSTERED INDEX CT_'+@tableName +'_'+@tableColunm+' ON '+@tableName+'('+@tableColunm+') ON '+@PSName+'('+@tableColunm+')'

   PRINT @strSql

   EXEC(@strSql)

   END   

ELSE IF @PKName IS  NULL

BEGIN

   SET @strSql='CREATE CLUSTERED INDEX CT_'+@tableName +'_'+@tableColunm+' ON '+@tableName+'('+@tableColunm+') ON '+@PSName+'('+@tableColunm+')'

   PRINT @strSql

   EXEC(@strSql)

END

END
go


create procedure [dbo].[SP_CreateHistoryTable_V1] 
(
	@dbName NVARCHAR(50),
	@tableName nvarchar(50)
) 
as
declare @sql varchar(1024),
@filePath nvarchar(256),
@fileName nvarchar(128)
set nocount ON

SELECT @filePath=Value FROM Settings WHERE Item = 'FileGroupPath'
IF @filePath IS NULL
	SET @filePath = 'E:\MSSQL_Data\db\LPBSS\'

if  object_id(@tableName) is  null
begin
	if not exists(select 1 from  sysfilegroups where groupname =@tableName)
	begin
		--创建文件组
		set @sql=' alter database ['+@dbName+'] add filegroup['+@tableName+']'
		exec(@sql)
	end
	--在文件组上创建表
	set @sql='CREATE TABLE [dbo].['+@tableName+'](
	[CardNum] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NOT NULL,
	[CardType] [int]  NOT NULL,
	[WorkSiteId] [int] NOT NULL,
	[SeqNum] [int] NOT NULL,
	[rssi] [int] NULL,
	[Distance] [float] NULL,
	[OccTime] [datetime] NOT NULL,
	[InDBTime] [datetime] DEFAULT (getdate()),
    [Status] [int] DEFAULT ((0))
	) on ['+@tableName+']'
	exec (@sql)
	set @sql=' create CLUSTERED index i_OccTime on  [dbo].['+@tableName+'] (OccTime)  with fillfactor = 100 '
	exec (@sql)
end
--在文件组上创建文件
if not exists(select 1 from sysfiles where  name =@tableName)
begin
	set @fileName = @filePath+@tableName+'.ndf'
	set @sql='alter database ['+@dbName+'] add file(name='''+@tableName+''' ,filename='''+@fileName+''') to filegroup['+@tableName+']'
	exec(@sql)
end
/*######################################################*/


go

CREATE procedure [dbo].[SP_CreateHistoryTable_nanotron] 
(
	@dbName NVARCHAR(50),
	@tableName nvarchar(50)
) 
as
declare @sql varchar(1024),
@filePath nvarchar(256),
@fileName nvarchar(128)
set nocount ON

SELECT @filePath=Value FROM Settings WHERE Item = 'FileGroupPath'
IF @filePath IS NULL
	SET @filePath = 'E:\MSSQL_Data\db\LPBSS\'

if  object_id(@tableName) is  null
begin
	if not exists(select 1 from  sysfilegroups where groupname =@tableName)
	begin
		--?????????
		set @sql=' alter database ['+@dbName+'] add filegroup['+@tableName+']'
		exec(@sql)
	end
	--??????????????
	set @sql='CREATE TABLE [dbo].['+@tableName+'](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[CardNum] [nvarchar](50) COLLATE Chinese_PRC_CI_AS NOT NULL,
	[SeqNum] [int] NOT NULL,
	[Worksite1] [int]  NULL,
	[Distance1] [float] NULL,
	[Worksite2] [int] NULL,
	[Distance2] [float] NULL,
	[Worksite3] [int] NULL,
	[Distance3] [float] NULL,
	[Worksite4] [int] NULL,
	[Distance4] [float] NULL,
	[Worksite5] [int] NULL,
	[Distance5] [float] NULL,
	[Worksite6] [int] NULL,
	[Distance6] [float] NULL,
    [TickTime] [bigint] NULL,
	[OccTime] [datetime] NULL,
	[InDBTime] [datetime] DEFAULT (getdate())
	) on ['+@tableName+']'
	exec (@sql)
	set @sql=' create CLUSTERED index i_id on  [dbo].['+@tableName+'] (id)  with fillfactor = 100 '
	exec (@sql)
end
--???????????????
if not exists(select 1 from sysfiles where  name =@tableName)
begin
	set @fileName = @filePath+@tableName+'.ndf'
	set @sql='alter database ['+@dbName+'] add file(name='''+@tableName+''' ,filename='''+@fileName+''') to filegroup['+@tableName+']'
	exec(@sql)
end
/*######################################################*/


go


create procedure [dbo].[SP_DropHistoryTable] 
(
	@dbName NVARCHAR(50),
 	@tableName varchar(50)
) 
as
declare @sql   varchar(1024)

--删除表
if not object_id(@tableName) is null
begin
	set @sql = 'drop table [dbo].['+@tableName+']'
	exec(@sql)    
end

--删除文件,文件组
if exists(select 1 from sysfiles where name=@tableName )
begin
    set @sql=' alter database '+@dbName+' remove file['+@tableName+']'       --删除文件
    exec(@sql)
end
if exists(select 1 from sysfilegroups where groupname= @tableName)   
begin   
     set @sql=' alter database '+@dbName+' remove filegroup['+@tableName+']' --删除文件组
     exec(@sql)
end



go


create procedure dbo.SP_InsertCardLocation (
 	@CardNum nvarchar(50),
	@CardType int,
 	@WorkSiteId int,
 	@x float,
 	@y float,
 	@z float,
 	@OccTime datetime,
    @Distance float = 0
 ) as
BEGIN
	SET NOCOUNT ON;
	if exists(select * from CardLoc where CardNum = @CardNum AND CardType = @CardType)
		update CardLoc set WorkSiteId = @WorkSiteId, x = @x, y = @y, z = @z, OccTime = @OccTime, Distance = @Distance where CardNum = @CardNum AND CardType = @CardType
	else
		insert into CardLoc(CardNum,CardType,WorkSiteId, x, y, z, OccTime, Distance) values(@CardNum,@CardType,@WorkSiteId, @x, @y, @z, @OccTime, @Distance)
END
go


Create procedure [dbo].[SP_InsertCardLocation_2D] (
 	@CardNum nvarchar(50),
	@CardType int,
 	@WorkSiteId int,
 	@x float,
 	@y float,
 	@OccTime datetime,
    @Distance float = 0
 ) as
BEGIN
	SET NOCOUNT ON;
	if exists(select * from CardLoc_2D where CardNum = @CardNum AND CardType = @CardType)
		update CardLoc_2D set WorkSiteId = @WorkSiteId, x = @x, y = @y, OccTime = @OccTime, Distance = @Distance where CardNum = @CardNum AND CardType = @CardType
	else
		insert into CardLoc_2D(CardNum, CardType, WorkSiteId, x, y, OccTime, Distance) values(@CardNum, @CardType, @WorkSiteId, @x, @y, @OccTime, @Distance)
END

GO

create procedure dbo.SP_InsertCardLowPower (
 	@CardNum nvarchar(50),
	@CardType int,
 	@Power nvarchar(50),
 	@WorkSiteId INT,
 	@OccTime datetime
 ) as
BEGIN
	SET NOCOUNT ON;
	DECLARE @id INT,
			@IsRead INT;
    SELECT TOP 1 @id = id, @IsRead=IsRead from CardLowPower where CardNum=@CardNum AND CardType=@CardType ORDER BY id DESC;
	if @id is NULL OR @IsRead = 1
		insert into CardLowPower(CardNum, CardType, Power, WorkSiteId, OccTime) values(@CardNum, @CardType, @Power, @WorkSiteId, @OccTime)
	else
		update CardLowPower set Power=@Power, WorkSiteId=@WorkSiteId, OccTime=@OccTime where id=@id
END

go

create procedure dbo.SP_InsertCardSos (
 	@CardNum nvarchar(50),
	@CardType int,
 	@WorkSiteId int,
 	@OccTime dateTime,
    @LastTime dateTime
 ) as
BEGIN
	SET NOCOUNT ON;
	DECLARE @id INT, @IsRead INT;
	SET @id = NULL;
	
	SELECT TOP 1 @id = id, @IsRead = IsRead from CardSos where CardNum = @CardNum AND CardType = @CardType ORDER BY id DESC
	if @id is NULL OR @IsRead = 1
		insert into CardSos(CardNum,CardType, WorkSiteId, OccTime, LastTime) values (@CardNum,@CardType, @WorkSiteId, @OccTime, @LastTime)
	else
		update CardSos set WorkSiteId = @WorkSiteId, LastTime = @LastTime where id = @id
END
go


CREATE procedure [dbo].[SP_InsertNurseRelation] (
 	@CustomerId int,
 	@NurseId int
 )
 
  as
BEGIN
	SET NOCOUNT ON;
	 declare @id int
	 select @id=@CustomerId from NurseRelation where CustomerId=@CustomerId and NurseId=@NurseId
	  if @id is  null
	  insert into NurseRelation(CustomerId,NurseId) values(@CustomerId,@NurseId)
END


GO


CREATE procedure [dbo].[SP_InsertOldPerson] (
 	@NurseUnitId int,
 	@personId int
 )
 
  as
BEGIN
	SET NOCOUNT ON;
	declare @num int
	select @num=NurseUnitId from Customer where PersonId=@personId 
	
	if @num is null
	  insert into Customer(PersonId,NurseUnitId) values(@personId,@NurseUnitId)
	else
	    if @num !=@NurseUnitId
	    begin
	    update Customer set NurseUnitId=@NurseUnitId where PersonId=@personId
	    end
END

GO



CREATE procedure [dbo].[SP_InsertNurseUnit] (
	@PersonId int,
 	@aName nvarchar(100),
 	@address nvarchar(100)
 )
 
  as
BEGIN
	SET NOCOUNT ON;
	DECLARE @id int	
	declare @nurseUnitId int
	select @nurseUnitId=NurseUnitId from AllPerson_view_2D where Id=@PersonId
	
	if @nurseUnitId is null
	  begin
	   select @id=id from Area where aName=@aName
	    if @id is  null
	      select @id=0
	   insert into NurseUnit(Address,AreaId) values(@address,@id)
	   set  @nurseUnitId=@@identity
	   insert into Customer(PersonId,NurseUnitId) values(@personId,@nurseUnitId)
	   end
   else
       select @id=id from Area where aName=@aName
	    if @id is  null
	    select @id=0
     update NurseUnit set AreaId=@id ,Address=@address where id=@nurseUnitId
END


GO


CREATE procedure [dbo].[SP_InsertNurseUnitByName] (
	@PersonId int,
 	@NurseName nvarchar(100)
 )
 
  as
BEGIN
	SET NOCOUNT ON;
	DECLARE @id int	
	
	
	select @id=Id from AllPerson_view_2D where PersonName=@NurseName
	
	if @id is not null
       insert into NurseRelation(CustomerId,NurseId) values(@PersonId,@id)
END


GO



create procedure [dbo].[De_NurseRelationByName] (
	@PersonId int
 )
 
  as
BEGIN
	SET NOCOUNT ON;
	delete from NurseRelation where CustomerId=@PersonId
END


GO

CREATE procedure [dbo].[SP_InsertLinkInfo] (
 	@WorkSiteId INT,
 	@Port INT,
 	@ConnDevNum INT,
 	@OccTime DATETIME
 ) as
BEGIN
	SET NOCOUNT ON;
	
	INSERT INTO dbo.LinkHistory(WorkSiteId, Port, ConnDevNum, OccTime) VALUES(@WorkSiteId, @Port, @ConnDevNum, @OccTime)
	
	IF EXISTS(SELECT * FROM dbo.LinkInfo WHERE ConnDevNum = @ConnDevNum)
		UPDATE dbo.LinkInfo SET WorkSiteId = @WorkSiteId, Port = @Port, OccTime = @OccTime WHERE ConnDevNum = @ConnDevNum
	ELSE
		INSERT INTO dbo.LinkInfo(WorkSiteId, Port, ConnDevNum, OccTime) VALUES(@WorkSiteId, @Port, @ConnDevNum, @OccTime)
END
go


create procedure dbo.SP_RunFileGroup as
BEGIN
DECLARE @temp TABLE
(
RowNumber INT,
item VARCHAR(255),
itemValue VARCHAR(255),
DESCRIPTION VARCHAR(255)
) 

INSERT INTO @temp
        ( RowNumber ,
          item ,
          itemValue ,
          DESCRIPTION
        )

SELECT ROW_NUMBER() OVER(ORDER BY Item) AS RowNumber, Item as item, Value as itemValue, Description as DESCRIPTION FROM Settings WHERE UPPER(RTRIM(LTRIM(Description)))=UPPER('table') 

Declare @maxRow int    --用来获得最大的rowNumber 
Declare @rowNo int 
DECLARE @tableName VARCHAR(255)
DECLARE @ColName VARCHAR(255)
Select @maxRow=max(rownumber) from @temp 
set @rowNo=1 

While @rowNo<=@maxRow  --用来对每一个rowNumber来进行循环操作
Begin 
    --此处对每一行要进行的操作的代码
    SELECT @tableName=LTRIM(RTRIM(item)),@ColName=LTRIM(RTRIM(itemValue)) FROM @temp WHERE RowNumber=@rowNo
    EXEC SP_AddFileGroup @tableName,@ColName
    PRINT @tableName+'  '+@ColName
    Set @rowNo=@rowNo+1 
End 

END
go


Create  proc up_getHistoryPathLine
(
  @group  int, --查询方式 1:按卡号,2:按姓名,3:按工号
  @strName     varchar(50),   --参数
  @beginTime   datetime ,--开始时间
  @endTime     datetime  --结束时间
)
as 
declare @tempCardNumber table
( 
	cardNum varchar(30)
)
declare @tempWorkSitePass table
(
	id    int,
    WorkSiteId int,
	Stime		datetime,
	eTime		datetime,
    cardNumber	varchar(50),
	CardType	int,
	x			float,
	y			float,
	z			float,
	x2          float,
	y2          float
)

begin
  SET NOCOUNT ON;

  if @group = 1
    insert into @tempCardNumber select @strName;
  else if @group = 2 
    insert into @tempCardNumber  select cardNumber from person where personName = @strName
  else if @group = 3
    insert into @tempCardNumber  select cardNumber from dbo.EmployeeView as empvw where number = @strName
  
	insert into @tempWorkSitePass select worksitepass.* from worksitepass inner join (select cardNum from @tempCardNumber) as cn
		    on worksitepass.CardNum=cn.cardNum 
			where stime >= @beginTime and stime <= @endTime order by stime asc;

    select tws.*,empvw.personName,d.name from @tempWorkSitePass tws left join dbo.EmployeeView as empvw on
			 tws.cardNumber=empvw.CardNumber left join department d on d.id=empvw.departmentId;

drop table #tempCardNumber;
drop table #tempWorkSitePass;
end
go



Create procedure [dbo].[up_UpdateCorAreaWorksite](@workSiteId int,@areaId int)
as
begin
  SET NOCOUNT ON;
  if exists(select * from corAreaWorksite where worksiteId = @workSiteId)
	update corAreaWorksite set areaId = @areaId where worksiteId = @workSiteId;
  else
    insert into corAreaWorksite select @workSiteId,@areaId;
end
go


---统计人数
CREATE procedure [dbo].[up_statisticsPersonNum]
(
  @timeOut int,		---设置时间间隔 
  @currentUser varchar(50) 
)
as
	--- 基站下人数统计
	declare @worksiteStatistic table
	(
		worksiteId int,
		worksiteAddress nvarchar(50),
		employeeCount  INT,
		customerCount INT
	)
	--- 区域下人数统计
	declare @areaStatistic table
	(
		areaId		int,
		areaName    nvarchar(50),
		employeeCount  INT,
		customerCount INT
	)

	--- 区域下人数统计
	declare @departmentStatistic table
	(
		departmentid int,
		departmentName nvarchar(50),
		personNum   int
	)
	--- 班组下人数统计
	declare @groupStatistic table
	(
		groupid int,
		groupName nvarchar(50),
		personNum   int
	)
begin
set nocount on;
	--if @currentUser = ''
	-- set @currentUser = 'admin';
	select * into #assayloc from RealTimePersonList_Vw where datediff(s,occtime,getdate()) < @timeOut
	select * into #employeeloc from RealTimeEmployeeList_Vw where datediff(s,occtime,getdate()) < @timeOut
	--and departmentId in 
	--(select udc.departId from userLogin ul left join userDepartConfig udc on ul.userName=udc.userName 
	--  where ul.userName = @currentUser);

	---按基站统计人数
	insert into @worksiteStatistic
	select w.worksiteid, w.address, (select count(1) from #assayloc a where a.worksiteid=w.worksiteid AND a.iscustomer=0) employeeCount,
			(select count(1) from #assayloc a where a.worksiteid=w.worksiteid AND a.iscustomer=1) customerCount
	from worksite w order by w.worksiteid

	---按区域统计人数
	insert into @areaStatistic 
	select area.id, area.aName,
	(select count(1) from #assayloc l where l.iscustomer=0 AND l.areaid = area.id or l.areaid in 
	(select id from area a where a.parentId = area.id) or
	l.areaid in (select id from area a1 where a1.parentId in 
	(select Id from area a2 where a2.parentId = area.id))) employeeCount,
	(select count(1) from #assayloc l where l.iscustomer=1 AND l.areaid = area.id or l.areaid in 
	(select id from area a where a.parentId = area.id) or
	l.areaid in (select id from area a1 where a1.parentId in 
	(select Id from area a2 where a2.parentId = area.id))) customerCount 
	from area order by area.id


	---按部门统计人数
	insert into @departmentStatistic
	select department.id,department.name,
	(select count(1) from #employeeloc rtperson where rtperson.departmentid = department.id or rtperson.departmentid in 
	(select id from department d where d.parentId = department.id) or
	rtperson.departmentId in (select id from department d1 where d1.parentId in 
	(select Id from department d2 where d2.parentId = department.id))) personCount 
	from department order by department.id

	---按班组统计人数
	insert into @groupStatistic
	select classTeam.id,classTeam.name,
	(select count(1) from #employeeloc rtperson where rtperson.classteamId=classTeam.id 
				and rtperson.departmentid=classteam.departId)
	from classTeam order by classTeam.id

	select worksiteid,employeeCount,customerCount from @worksiteStatistic
	select areaName,employeeCount,customerCount from @areaStatistic
	select departmentName,personNum from @departmentStatistic
	select groupName,personNum from @groupStatistic

drop table #assayloc
drop table #employeeloc
end
go

---统计人数
Create procedure [dbo].[up_statisticsGoodsNum]
(
  @timeOut int,		---设置时间间隔
  @currentUser varchar(50) 
)
as
--- 基站下人数统计
declare @worksiteStatistic table
(
	worksiteId int,
	worksiteAddress nvarchar(50),
    personNum  int 
)
--- 区域下人数统计
declare @areaStatistic table
(
	areaId		int,
	areaName    nvarchar(50),
	personNum	int  
)

--- 区域下人数统计
declare @departmentStatistic table
(
    departmentid int,
	departmentName nvarchar(50),
    personNum   int
)
begin
set nocount on;
if @currentUser = ''
 set @currentUser = 'admin';
select * into #assayloc from RealTimeGoodsList_Vw where datediff(s,occtime,getdate()) < @timeOut and
departmentId in 
(select udc.departId from userLogin ul left join userDepartConfig udc on ul.userName=udc.userName 
  where ul.userName = @currentUser);

---按基站统计人数
insert into @worksiteStatistic
select worksiteid,address, (select count(1) from #assayloc rtperson 
				where rtperson.worksiteid = worksite.worksiteid) personCount
from worksite order by worksiteid

---按区域统计人数
insert into @areaStatistic 
select area.id,area.aName,
(select count(1) from #assayloc rtperson where rtperson.areaid = area.id or rtperson.areaid in 
(select id from area a where a.parentId = area.id) or
rtperson.areaid in (select id from area a1 where a1.parentId in 
(select Id from area a2 where a2.parentId = area.id))) personCount 
from area order by area.id


---按部门统计人数
insert into @departmentStatistic
select department.id,department.name,
(select count(1) from #assayloc rtperson where rtperson.departmentid = department.id or rtperson.departmentid in 
(select id from department d where d.parentId = department.id) or
rtperson.departmentId in (select id from department d1 where d1.parentId in 
(select Id from department d2 where d2.parentId = department.id))) personCount 
from department order by department.id

select worksiteid,worksiteAddress,personNum from @worksiteStatistic
select areaId,areaName,personNum from @areaStatistic
select departmentid,departmentName,personNum from @departmentStatistic

drop table #assayloc
end
go


CREATE procedure [dbo].[up_getAttributeNodeTree]
(
	@statisMethod	int,		---0  区域节点属性  1 基站节点属性   2人节点属性  3部门节点属性  4 班组节点属性
	@NodeId			int,
	@PersontimeOut	int			-- 人员超时的时间(单位秒)
)
as
begin
set nocount on;
if @statisMethod = 0
begin
	select distinct area.aName,areaType.name as areaTypeName,rightArea.aName as parentName,ws.worksiteid,ws.address into #areaNode from area left join areaType on
	 area.type=areaType.id left join (select * from area)  as rightArea on area.parentId=rightArea.id
	left join corAreaWorksite caw on caw.areaId=area.id left join worksite ws on caw.worksiteid=ws.worksiteid
	where area.id = @NodeId; --group by area.aName
  
	--区域属性
    select aName,areaTypeName,parentName from #areaNode group by aName,areaTypeName,parentName;

    --统计区域下基站
    select worksiteid,address from #areaNode;
end
else if @statisMethod = 1
begin
	--select ws.*,li.worksiteid as tcuId,caw.areaid,cardloc.CardNum,p.personName into #worksiteNode from worksite ws left join cardloc on ws.worksiteid=cardloc.worksiteid inner join 
--person p on p.CardNumber=Cardloc.CardNum left join linkInfo li on ws.worksiteid=li.ConnDevNum left join corAreaWorksite caw on caw.worksiteid=ws.worksiteid
 --where ws.worksiteid=@NodeId and datediff(s,cardLoc.occTime,getdate()) < @PersontimeOut;
	select * into #worksiteNode from (
		select distinct ws.*,li.worksiteid as tcuId,caw.areaid,cardloc.CardNum,cardloc.CardType,p.personName from worksite ws left join cardloc on ws.worksiteid=cardloc.worksiteid 
		and (cardloc.CardType=3 or CardLoc.CardType=6 or CardLoc.CardType=7) inner join 
	person p on p.CardNumber=Cardloc.CardNum left join linkInfo li on ws.worksiteid=li.ConnDevNum left join corAreaWorksite caw on caw.worksiteid=ws.worksiteid
	 where ws.worksiteid=@NodeId and datediff(s,cardLoc.occTime,getdate()) < @PersontimeOut union
	select distinct ws.*,li.worksiteid as tcuId,caw.areaid,cardloc.CardNum,cardloc.CardType,a.name from worksite ws left join cardloc on ws.worksiteid=cardloc.worksiteid and cardloc.CardType=4 inner join 
	articles a on a.CardNumber=Cardloc.CardNum left join linkInfo li on ws.worksiteid=li.ConnDevNum left join corAreaWorksite caw on caw.worksiteid=ws.worksiteid
	 where ws.worksiteid=@NodeId 
		) as temp
	if not exists(select * from #worksiteNode)
	begin
		select ws.worksiteid,address,type,Dimension,x,y,z,rotaryw,rotaryX,rotaryY,rotaryZ,visualOffsetX,visualOffsetY,visualOffsetZ
				,li.worksiteid,areaId from worksite ws left join linkinfo li on ws.worksiteid=li.ConnDevNum 
				left join corAreaWorksite caw on caw.worksiteid=ws.worksiteid where ws.worksiteid=@NodeId
	end
	else
	begin	
	--基站属性
	select worksiteid,address,type,Dimension,x,y,z,rotaryw,rotaryX,rotaryY,rotaryZ,visualOffsetX,visualOffsetY,visualOffsetZ,
	tcuId,areaId from #worksiteNode group by worksiteid,address,type,Dimension,x,y,z,rotaryw,rotaryX,rotaryY,rotaryZ,visualOffsetX,visualOffsetY,
    visualOffsetZ,tcuId,areaId;
	end

	--基站下人员统计
	select CardNum,CardType,personName from #worksiteNode;
end
else if @statisMethod = 2
	--人员属性
    select *  from person where cardNumber = convert(varchar(30),@NodeId);
else if @statisMethod = 3
begin
    select dt.*,rightDepart.name as parentName,ct.id as classId,ct.name as className into #departNode 
			from department dt left join (select * from department) as rightDepart 
			on dt.parentid=rightDepart.id left join classTeam ct on ct.departid=dt.id
			where dt.id= @NodeId;
--left join person p on dt.id=p.departmentid inner join cardLoc 
--			on p.cardNumber=cardLoc.cardNum 
	--部门属性
	select id,name,leader,telephone,parentName 
		from #departNode group by id,name,leader,telephone,parentName;
   
	select classId,className from #departNode;
end
else if @statisMethod = 4
begin
    select ct.*,p.CardNumber,p.personName into  #classTeamNode from person p
	inner join cardLoc on p.CardNumber=cardLoc.CardNum and  p.CardType=cardLoc.CardType 
	inner join Employee on Employee.personId = p.id
	inner join classTeam ct on ct.id=Employee.classTeamId 
	inner join department dt on ct.departid=dt.id 
	and dt.id=Employee.departmentid where ct.id=@NodeId and datediff(s,cardLoc.occTime,getdate()) < @PersontimeOut;
	if not exists(select * from #classTeamNode)--判断是否有结果集
	begin
		select id,name,departId,leader,telephone from classTeam where id=@NodeId
	end
    else
	begin
	--班组属性
	select id,name,departId,leader,telephone from #classTeamNode 
			group by id,name,departId,leader,telephone;
	end

	select cardNumber,personName from #classTeamNode;
end
if exists(select * from sysobjects where id=object_id('#areaNode'))
 drop table #areaNode;
if exists(select * from sysobjects where id=object_id('#worksiteNode'))
 drop table #worksiteNode;
if  exists(select * from sysobjects where id=object_id('#departNode'))
 drop table #departNode;
if exists(select * from sysobjects where id=object_id('#classTeamNode'))
 drop table #classTeamNode;
end 
go


Create proc [dbo].[up_getAreaUnderBaseStation_Map2D] ( @areaId int )
as 
	declare @tempArea table 
	(
		areaId int
	)	
declare @flagAreaId int
declare @Flag int
set nocount on;
begin
	set @Flag = 1;
	set @flagAreaId = @areaId;
	insert into @tempArea select @flagAreaId;
	while (@Flag = 1)	--len(@flagAreaId) <> 0
	begin
		select @flagAreaId=parentId from area where id = @flagAreaId;
		if exists(select parentId from area where id = @flagAreaId)
		begin
			set @Flag = 1;
			insert into @tempArea select @FlagAreaId;
		end
		else 
			set @Flag = 0;
	end 
	--insert into @tempArea select @flagAreaId;
	--select * from @tempArea

	select ws.worksiteId,x,y,z,VisualOffsetX,VisualOffsetY,VisualOffsetZ
				   from @tempArea tA inner join corAreaWorkSite caw on ta.areaId=caw.areaId inner join workSite ws
					on ws.workSiteId=caw.worksiteId
end
Go

Create proc [dbo].[up_areaAlarm] 
as
	set nocount on;
	begin 
	select id,aName,Name,cardNum,personName,convert(varchar(23),beginTime,120) as 
	beginTime,convert(varchar(23),endTime,120) as endTime,isRead into #tempareaAlarm
												   from AreaAlarmMsg_Vw ;
	select * from #tempareaAlarm  aaMsg where (select count(1) 
	from #tempareaAlarm where cardNum=aaMsg.cardNum 
												   and beginTime>aaMsg.beginTime)<1 and (isRead = 0 or endTime is null) 
	order by beginTime desc;

	if exists(select * from sysobjects where id=object_id('#tempareaAlarm'))
	 drop table #tempareaAlarm;
end
Go

/*==============================================================*/
/* procedures:	历史轨迹播放轨迹点								*/
/*==============================================================*/
CREATE proc [dbo].[up_CoordinatesPointHistoryTrack] 
(
		 @cardNumber int,
		 @cardType	 int,
		 @stime		datetime,
		 @etime		datetime
)
as
begin
---轨迹详细信息
	declare  @tempPathLine table
	(
		cardNumber  nvarchar(50),
		personName  nvarchar(50),
		departName	nvarchar(50),
		officePost	nvarchar(50),
		worksiteAddress nvarchar(50),
		--areaName		nvarchar(50),
		worksiteid	int,
		STime		datetime,
		ETime		datetime,
		x			float,
		y			float,
		z			float
	)
set nocount on;
DECLARE @CardModel INT;			--- 卡的分区
DECLARE @table_name VARCHAR(50);	--- 表名
DECLARE @date_suffixes VARCHAR(10);	--- 历史表的后缀日期名
DECLARE @count INT;					--- 日期记数
DECLARE @day INT;					--- 日期天数
DECLARE @Sql NVARCHAR(1000);			--- 动态sql语句
DeClARE @Flag INT;					--- 查询标志位
SET @count = 0;
SET @table_name = 'dbo.Loc_3D_';		--- 历史轨迹动态表名的前缀
SET @CardModel = @cardNumber % 10;		--- 增加条件提高查询的效率
SET @day = DATEDIFF(d,@stime,@etime);	--- 经过的时间天数
SET @Flag = 0;
BEGIN
		IF( 0 = @day)		--- 判断开始时间和结束时间是否同一天
	BEGIN
		SET @date_suffixes = CONVERT(VARCHAR(10),@stime,112);		--- 转换日期格式yyyymmhh
		SET @table_name = @table_name + @date_suffixes;				--- 表名转换为动态可增加表
			SET @sql = 'select cin.cardNum,p.personName,dt.name as departName,op.name as officePost,
			ws.address,cin.worksiteid,cin.OccTime,cin.OccTime,cin.x,cin.y,cin.z from '+@table_name+' as cin 
			inner join person p on cin.cardNum=p.cardNumber left join worksite ws on ws.worksiteid=cin.worksiteid 
			LEFT JOIN dbo.department dt ON p.departmentId = dt.id LEFT JOIN classTeam ct ON p.classTeamId = ct.id AND 
			dt.id = ct.departId LEFT JOIN officePosition op ON op.id = p.officePositionId 
			where cin.cardNum='''+CONVERT(VARCHAR(30),@cardNumber)+''' AND cin.CardModel='+CONVERT(VARCHAR(30),@CardModel)
			+' and (cin.cardType = 3 or cin.cardType = 6 or cin.cardType = 10) and cin.occTime >= '''+CONVERT(VARCHAR(30),@stime,120)+
			'''and cin.occTime <= '''+CONVERT(VARCHAR(30),@etime,120)+''' order by cin.occTime asc';
		--EXECUTE(@Sql);
		INSERT INTO @tempPathLine(cardNumber,personName,departName,officePost,worksiteAddress,worksiteid,STime,ETime,x,y,z) 
		EXECUTE sp_executesql @sql;		--- 执行动态sql并插入到临时表中
	END
	ELSE BEGIN
		WHILE(@count <= @day)	--- 循环查询
		BEGIN
			SET @table_name = 'dbo.Loc_3D_';		--- 重置表明前缀
			SET @date_suffixes = CONVERT(VARCHAR(10),DATEADD(DAY,@count,@stime),112);	--- 增加天数并转换日期格式yyyymmhh
			SET @table_name = @table_name + @date_suffixes;				--- 表名转换为动态可增加表
			DECLARE @FLag_sql NVARCHAR(200);
			SET @FLag_sql = 'IF EXISTS(select * from sysobjects where id = object_id('''+@table_name+''') and type in (''U'')) 
								select @Flag=1 else select @Flag=0';		--- 判断表是否存在
			EXECUTE sp_executesql @FLag_sql,N'@Flag int output',@Flag OUTPUT;
			IF (@Flag = 1)		--- 表存在
			BEGIN
				SET @sql = 'select cin.cardNum,p.personName,dt.name as departName,op.name as officePost,
				ws.address,cin.worksiteid,cin.OccTime,cin.OccTime,cin.x,cin.y,cin.z  from '+@table_name+' as cin 
				inner join person p on cin.cardNum=p.cardNumber left join worksite ws on ws.worksiteid=cin.worksiteid 
				LEFT JOIN dbo.department dt ON p.departmentId = dt.id LEFT JOIN classTeam ct ON p.classTeamId = ct.id AND 
				dt.id = ct.departId LEFT JOIN officePosition op ON op.id = p.officePositionId 
				where cin.cardNum='''+CONVERT(VARCHAR(30),@cardNumber)+''' AND cin.CardModel = '+CONVERT(VARCHAR(30),@CardModel)+
				' and (cin.cardType = 3 or cin.cardType = 6) and cin.occTime >= '''+CONVERT(VARCHAR(30),@stime,120)+
				''' and cin.occTime <= '''+CONVERT(VARCHAR(30),@etime,120)+''' order by cin.occTime asc';
				--EXECUTE(@Sql);
				INSERT INTO @tempPathLine(cardNumber,personName,departName,officePost,worksiteAddress,worksiteid,STime,ETime,x,y,z) 
				EXECUTE sp_executesql @sql;		--- 执行动态sql并插入到临时表中
			END
			SET @Flag = 0;					--- 重置标志
			SET @count = @count + 1;		--- 查询的天数
		END
	END
END
			SELECT cardNumber,personName,departName,officePost,worksiteAddress,
			worksiteid,CONVERT(VARCHAR(23),STime,120) as STime,CONVERT(VARCHAR(23),ETime,120) as ETime,x,y,z 
			FROM @tempPathLine ORDER BY STime ASC
END
Go

/****** Object:  StoredProcedure [dbo].[up_WorkSitePassHistoryTrack]    Script Date: 09/24/2014 15:38:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
/*==============================================================*/
/* procedures:	历史轨迹播放基站经过								*/
/*==============================================================*/
CREATE proc [dbo].[up_WorkSitePassHistoryTrack] 
(
		 @cardNumber int,
		 @cardType	 int,
		 @stime		datetime,
		 @etime		datetime
)
as
BEGIN
	set nocount on;

	DECLARE @Sql NVARCHAR(1024) --dt.name as departName,op.name as officePost, LEFT JOIN dbo.department dt ON p.departmentId = dt.id 
	--LEFT JOIN classTeam ct ON p.classTeamId = ct.id AND dt.id = ct.departId LEFT JOIN officePosition op 
	--ON op.id = p.officePositionId
	SET @sql = 'select cin.cardNum,p.personName,d.name,op.name,ws.address,cin.worksiteid,
	CONVERT(varchar(50), cin.STime, 120) as STime,CONVERT(varchar(50), cin.ETime, 120) as ETime,cin.x,cin.y,cin.z 
	from WorkSitePass as cin inner join person p on cin.cardNum=p.cardNumber left join worksite ws  
	on ws.worksiteid=cin.worksiteid left join department d on p.departmentId = d.id left join officePosition op 
    on p.officePositionId = op.id where cin.cardNum='''+CONVERT(VARCHAR(30),@cardNumber)+''' 
	AND (cin.cardType = 3 or cin.cardType = 6 or cin.cardType = 10) and cin.STime >= '''+CONVERT(VARCHAR(30),@stime,120)
	+'''and cin.STime <= '''+CONVERT(VARCHAR(30),@etime,120)+''' order by cin.STime asc';

	EXECUTE sp_executesql @sql
END

GO

/****** Object:  StoredProcedure [dbo].[up_CoordinatesPointHistoryPathLine]    Script Date: 09/24/2014 15:39:27 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
/*==============================================================*/
/* procedures:	历史轨迹总入口								*/
/*==============================================================*/
CREATE proc [dbo].[up_CoordinatesPointHistoryPathLine] 
(
		 @cardNumber int,
		 @cardType	 int,
		 @stime		datetime,
		 @etime		datetime
)
as
begin
	set nocount on;
	
	DECLARE @trackType VARCHAR(10)
	SELECT @trackType = Value FROM dbo.Settings WHERE Item = 'HisTrackType'
	
	IF @trackType = '1'
		EXEC up_CoordinatesPointHistoryTrack @cardNumber, @cardType, @stime, @etime
	ELSE
		EXEC up_WorkSitePassHistoryTrack @cardNumber, @cardType, @stime, @etime		
END

GO

CREATE procedure [dbo].[sp_InsertMattressAlarmSet]
@StartTime datetime,
@EndTime datetime,
@Status int
as
set nocount on
  declare @num int
  begin
   select @num=id  from MattressAlarmSet  where StartTime=@StartTime and
   EndTime=@EndTime and  Status=@Status
   if @num is null
     insert into MattressAlarmSet(StartTime,EndTime,Status) values(@StartTime,@EndTime,@Status)
  end
GO

create procedure [dbo].[sp_DeleteMattressAlarmSet]

as
set nocount on
 begin
 delete from dbo.MattressAlarmSet
  end
GO


CREATE procedure [dbo].[up_InsertArticles] (
 	@CardNum nvarchar(50),
 	@name nvarchar(50),
	@type tinyint,
	@Tag  nvarchar(50),
 	@partId tinyint,
 	@RegisterTime datetime,
	@TimeStamp datetime,
	@crc	 int,
	@Flag    int
 ) 
as
declare @tempCrc table
(
	crc			int,
	TimeStamp	datetime
)
BEGIN
	SET NOCOUNT ON;
	if exists(select * from Articles where CardNumber = @CardNum )
--		begin 
--			if @Flag = 1
--			begin
--				insert into Articles(CardNumber,name,type,Tag,departmentId,RegisterTime,TimeStamp,CRC) 
--					values(@CardNum,@name,@type, @Tag, @partId, @RegisterTime, @TimeStamp,@crc);
--			end
--			else 
			begin
				insert into @tempCrc select crc,TimeStamp from Articles where CardNumber = @CardNum;
				-- CRC不相同时，APP修改记录的时间和卡上报的时间差在一分钟之后才更新
				if exists(select * from @tempCrc where crc != @crc) 
					update Articles set name = @name, type = @type, tag = @Tag, departmentId = @partId,
							 RegisterTime = @RegisterTime,CRC=@crc,TimeStamp=@TimeStamp
					where CardNumber = @CardNum;
			end
--		end 
	else
		insert into Articles(CardNumber,name,type,Tag,departmentId,RegisterTime,TimeStamp,CRC) 
					values(@CardNum,@name,@type, @Tag, @partId, @RegisterTime,@TimeStamp,@crc);
END
go


Create FUNCTION [dbo].[f_Sort] ( @ID INT = 0, @sort INT = 1 )
RETURNS @t_Level TABLE ( ID CHAR(3), sort INT )
AS 
    BEGIN
        DECLARE tb CURSOR LOCAL
        FOR
            SELECT  ID
            FROM    ArticlesType
            WHERE   parentId = @ID
        --OR(@ID IS NULL AND PID IS NULL)
        OPEN TB
        FETCH tb INTO @ID
        WHILE @@FETCH_STATUS = 0 
            BEGIN
                INSERT  @t_Level
                VALUES  ( @ID, @sort )
                SET @sort = @sort + 1
                IF @@NESTLEVEL < 32 --如果递归层数未超过32层(递归最大允许32层)
                    BEGIN
            --递归查找当前节点的子节点
                        INSERT  @t_Level
                                SELECT  *
                                FROM    f_Sort(@ID, @sort)
                        SET @sort = @sort + @@ROWCOUNT  --排序号加上子节点个数
                    END
                FETCH tb INTO @ID
            END
        RETURN
    END 
Go

/* 权限绑定查看科室 */
Create TRIGGER [dbo].[lookObject_trigger_insert] ON [dbo].[department]
    INSTEAD OF INSERT
AS
SET IDENTITY_INSERT [department] ON;
begin
	Declare @departId	int,
			@name	 nvarchar(50),
			@userName	nvarchar(50)
	 
	if exists( select id from inserted where id > 0)
		insert into department(id,number,name,parentId,leader,telephone) select * FROM inserted;
     else 
	  begin
		if exists(select id from [department])
			set @departId = (select max(id) from [department]);
		else
			set @departId = 0;
			insert into department(id,number,name,parentId,leader,telephone) select @departId+1,number,name,parentId,leader,telephone from inserted;
	  end 

	SELECT  @departId = (select max(id) from [department]),
			@name = name
    FROM   inserted
   
	DECLARE @userTable table
	(
		userName	nvarchar(50)
	)
   insert into @userTable select ul.userName from Role inner join userRoleRelation urr on role.id=urr.roleId 
   inner join userLogin ul on urr.userId=ul.id where role.id in (1,2,3)

	DECLARE Cur CURSOR
        FOR
            SELECT userName  FROM  @userTable
        OPEN Cur
        FETCH NEXT FROM Cur INTO @userName
        WHILE @@fetch_status = 0 
            BEGIN
                INSERT  INTO userDepartConfig
                        ( userName ,
                          departId
                        )
					select @userName,@departId
                FETCH NEXT FROM Cur INTO @userName
            END   
        CLOSE Cur   
        DEALLOCATE Cur
end
Go

/* 物品换卡 */
Create TRIGGER [dbo].[ChangeCard_trigger_update_Goods] ON [dbo].[Articles]
    After UPDATE
AS
begin
 declare @bFlag int 
 set @bFlag = 1;
	 begin transaction
	  while(@bFlag = 1)
		begin
		  declare @errorSum int    
			if exists(select * from Articles where destoryTime IS NOT NULL)
			begin
				insert into HistoryArticles
				 select cardNumber,name,type,tag,departmentId,RegisterTime,destoryTime,
				TimeStamp,CRC,description,photoExtends,photo,id from Articles where destoryTime IS NOT NULL;
				set @errorSum=@errorSum+@@error  
				delete from Articles where cardNumber = (select cardNumber from inserted);
				set @errorSum=@errorSum+@@error 
				delete from CardInformation where cardNum = (select cardNumber from inserted) and CardType = 4;
				--- 只删除物品的卡信息
				set @errorSum=@errorSum+@@error 
			end
			if @errorSum>0
				begin
					rollback transaction;
				end
			else
				begin
					set @bFlag = 0;		-- 退出循环
					commit transaction;
				end
		end 
end
Go

/* 人员换卡 */
--Create TRIGGER [dbo].[ChangeCard_trigger_update_person] ON [dbo].[person]
--    After UPDATE
--AS
--begin
--	declare @bFlag int 
--	set @bFlag = 1;
--	begin transaction
--	  while(@bFlag = 1)
--		begin
--		  declare @errorSum int    
--			 if exists(select * from person where outTime IS NOT NULL)
--			begin
--				insert into HistoryPerson
--				select cardNumber,personName,sex,mobile,
--				telephone,identifyNum,photoExtends,photo,inTime,outTime,id from person where outTime IS NOT NULL;
--				set @errorSum=@errorSum+@@error  
--				delete from person where cardNumber = (select cardNumber from inserted);
--				set @errorSum=@errorSum+@@error 
--				delete from CardInformation where cardNum = (select cardNumber from inserted) and (CardType = 3 or CardType = 6);
--				--- 只删除人员的卡信息
--				set @errorSum=@errorSum+@@error 
--			end
--			if @errorSum>0
--				begin
--					rollback transaction;
--				end
--			else
--				begin
--					set @bFlag = 0;		-- 退出循环
--					commit transaction;
--				end
--		end 
--end
--Go

Create FUNCTION [dbo].[f_userLogin_Sort] ( @ID INT = 0, @sort INT = 1 )
RETURNS @t_Level TABLE ( ID CHAR(3), sort INT )
AS 
    BEGIN
        DECLARE tb CURSOR LOCAL
        FOR
            SELECT  ID
            FROM    userLogin
            WHERE   parentId = @ID
        --OR(@ID IS NULL AND PID IS NULL)
        OPEN TB
        FETCH tb INTO @ID
        WHILE @@FETCH_STATUS = 0 
            BEGIN
                INSERT  @t_Level
                VALUES  ( @ID, @sort )
                SET @sort = @sort + 1
                IF @@NESTLEVEL < 32 --如果递归层数未超过32层(递归最大允许32层)
                    BEGIN
            --递归查找当前节点的子节点
                        INSERT  @t_Level
                                SELECT  *
                                FROM    f_Sort(@ID, @sort)
                        SET @sort = @sort + @@ROWCOUNT  --排序号加上子节点个数
                    END
                FETCH tb INTO @ID
            END
        RETURN
    END 
Go

/** 获取物品类型下的所有	类型*/
Create procedure up_GoodsChildTypeSet(@typeId int)
as
SET NOCOUNT ON;
begin
	declare @tempGoodsType table 
	(
		goodsTypeId int
	)	
	declare @childTempGoodType table 
	(
		goodId			int,
		childGoodID1    int ,
		childGoodID2    int ,
		childGoodID3    int ,
		childGoodID4    int ,
		childGoodID5    int ,
		childGoodID6    int ,
		childGoodID7    int 
	)

	insert into @childTempGoodType select a1.id,a2.id,a3.id,a3.parentId,a4.id,a4.parentId,a5.id,a5.parentId
		from ArticlesType a1 left join ArticlesType a2 on a1.id=a2.parentid left join ArticlesType a3 on a2.id=a3.parentId 
		left join ArticlesType a4 on a3.id=a4.parentId left join ArticlesType a5 on a4.id=a5.parentid
		where a1.id = @typeId
	
	insert into @tempGoodsType select distinct goodId from @childTempGoodType union
	select childGoodID1 from @childTempGoodType  union 
	select childGoodID2 from @childTempGoodType  union 
	select childGoodID3 from @childTempGoodType  union 
	select childGoodID4 from @childTempGoodType  union 
	select childGoodID5 from @childTempGoodType  union
	select childGoodID6 from @childTempGoodType  union
	select childGoodID7 from @childTempGoodType 
	
   select * from @tempGoodsType where goodsTypeId is not null;
end
go

/** 获取区域下的子区域	限定7层*/
Create proc [dbo].[up_getAreaUnderChildArea] ( @areaId int )
as 
	declare @tempArea table 
	(
		areaId int
	)	
	declare @childTempArea table 
	(
		areaId			int,
		childAreaID1    int ,
		childAreaID2    int ,
		childAreaID3    int ,
		childAreaID4    int ,
		childAreaID5    int ,
		childAreaID6    int ,
		childAreaID7    int 
	)
set nocount on;
begin
	insert into @childTempArea select a1.id,a2.id,a3.id,a3.parentId,a4.id,a4.parentId,a5.id,a5.parentId
	from area a1 left join area a2 on a1.id=a2.parentid left join area a3 on a2.id=a3.parentId 
	left join area a4 on a3.id=a4.parentId left join area a5 on a4.id=a5.parentid
	where a1.id = @areaId
	
	insert into @tempArea select distinct areaId from @childTempArea union
	select childAreaID1 from @childTempArea  union 
	select childAreaID2 from @childTempArea  union 
	select childAreaID3 from @childTempArea  union 
	select childAreaID4 from @childTempArea  union 
	select childAreaID5 from @childTempArea  union
	select childAreaID6 from @childTempArea  union
	select childAreaID7 from @childTempArea  union
    select @areaId

	select * from @tempArea where areaId is not null;
end

Go

Create proc [dbo].[up_getDepartUnderChildDepart] ( @departId int )
as 
	declare @tempDepart table 
	(
		departId int
	)	
	declare @childTempDepart table 
	(
		departId		int,
		childDepartID1    int ,
		childDepartID2    int ,
		childDepartID3    int ,
		childDepartID4    int ,
		childDepartID5    int ,
		childDepartID6    int ,
		childDepartID7    int 
	)
set nocount on;
begin
	insert into @childTempDepart select a1.id,a2.id,a3.id,a3.parentId,a4.id,a4.parentId,a5.id,a5.parentId
	from department a1 left join department a2 on a1.id=a2.parentid left join department a3 on a2.id=a3.parentId 
	left join department a4 on a3.id=a4.parentId left join department a5 on a4.id=a5.parentid
	where a1.id = @departId
	
	insert into @tempDepart select distinct departId from @childTempDepart union
	select childDepartID1 from @childTempDepart  union 
	select childDepartID2 from @childTempDepart  union 
	select childDepartID3 from @childTempDepart  union 
	select childDepartID4 from @childTempDepart  union 
	select childDepartID5 from @childTempDepart  union
	select childDepartID6 from @childTempDepart  union
	select childDepartID7 from @childTempDepart  union
    select @departId

	select * from @tempDepart where departId is not null;
end
Go


Create proc [dbo].[up_CoordinatesPointPersonHistoryInfo] 
(
		 @cardNumber int,
		 @cardType	 int,
		 @stime		datetime,
		 @etime		datetime
)
as
begin
	---轨迹详细信息
	declare  @tempPathLine table
	(
		cardNumber  nvarchar(50),
		personName  nvarchar(50),
		departName	nvarchar(50),
		officePost	nvarchar(50),
		worksiteAddress nvarchar(50),
		--areaName		nvarchar(50),
		worksiteid	int,
		STime		datetime,
		ETime		datetime,
		x			float,
		y			float,
		z			float
	)
set nocount on;
	---卡号轨迹
			select cardNum,worksiteid,STime,ETime,x,y,z into #pathLinePoint from worksitepass 
			where cardNum = @cardNumber and cardType = @cardType and stime >= @stime 
			 and stime <= @etime 

select * into #tempWorkSitePass from (
--- person轨迹
--insert into @tempPathLine convert(varchar(23),STime,120),convert(varchar(23),ETime,120)
select pLp.cardNum,p.personName,dt.name as departName,op.name as officePost,
		ws.address,pLp.worksiteid,pLp.STime,pLp.ETime,pLp.x,pLp.y,pLp.z
from  #pathLinePoint pLp-- 一张卡的轨迹
inner join person p on pLp.cardNum=p.cardNumber
left join worksite ws on ws.worksiteid=pLp.worksiteid
LEFT JOIN dbo.department dt ON p.departmentId = dt.id LEFT  JOIN
 classTeam ct ON p.classTeamId = ct.id AND 
dt.id = ct.departId LEFT JOIN officePosition op ON op.id = p.officePositionId
where p.intime <= STime
union all
---> historyPerson 轨迹
select pLp.cardNum,ph.personName,dt.name as departName,op.name as officePost,
		ws.address,pLp.worksiteid,pLp.STime,pLp.ETime,pLp.x,pLp.y,pLp.z
from  #pathLinePoint pLp-- 一张卡的轨迹
inner join HistoryPerson ph on pLp.cardNum=ph.cardNumber
left join worksite ws on ws.worksiteid=pLp.worksiteid
LEFT JOIN dbo.department dt ON ph.departmentId = dt.id LEFT  JOIN
 classTeam ct ON ph.classTeamId = ct.id AND 
dt.id = ct.departId LEFT JOIN officePosition op ON op.id = ph.officePositionId
where ph.intime <= plp.Stime and ph.outTime >= plp.Stime
) t
order by stime asc
--p.intime < STime
--into #tempWorkSitePass
--union convert(varchar(23),occTime,120),convert(varchar(23),occTime,120)
select cardNum,worksiteid,cin.OccTime as STime,cin.OccTime as ETime,x,y,z into #detailPathLinePoint
 from CardLoc_Interval cin
where cardNum = @cardNumber and cardType = @cardType and cin.occTime >= @stime  
and cin.occTime <= @etime --order by cin.occTime asc

select * into #detailPathLine from 
(
select dpLp.cardNum,p.personName,dt.name as departName,op.name as officePost,
		ws.address,dpLp.worksiteid,dpLp.STime,dpLp.ETime,dpLp.x,dpLp.y,dpLp.z
from  #detailPathLinePoint dpLp-- 一张卡的轨迹
inner join person p on dpLp.cardNum=p.cardNumber
left join worksite ws on ws.worksiteid=dpLp.worksiteid
LEFT JOIN dbo.department dt ON p.departmentId = dt.id LEFT  JOIN
 classTeam ct ON p.classTeamId = ct.id AND 
dt.id = ct.departId LEFT JOIN officePosition op ON op.id = p.officePositionId
where p.intime <= STime
union all
---> historyPerson 轨迹
select dpLp.cardNum,ph.personName,dt.name as departName,op.name as officePost,
		ws.address,dpLp.worksiteid,dpLp.STime,dpLp.ETime,dpLp.x,dpLp.y,dpLp.z
from  #detailPathLinePoint dpLp-- 一张卡的轨迹
inner join HistoryPerson ph on dpLp.cardNum=ph.cardNumber
left join worksite ws on ws.worksiteid=dpLp.worksiteid
LEFT JOIN dbo.department dt ON ph.departmentId = dt.id LEFT  JOIN
 classTeam ct ON ph.classTeamId = ct.id AND 
dt.id = ct.departId LEFT JOIN officePosition op ON op.id = ph.officePositionId
where ph.intime <= dpLp.Stime and ph.outTime >= dpLp.Stime
)
t 
insert into @tempPathLine 
select * from #tempWorkSitePass union
select * from #detailPathLine 

select cardNumber,personName,departName,officePost,worksiteAddress,worksiteid,
convert(varchar(23),STime,120) as STime,convert(varchar(23),ETime,120) as ETime,x,y,z  
from @tempPathLine order by STime asc

--select * from #PathLine
if exists(select * from sysobjects where id=object_id('#pathLinePoint'))
 drop table #pathLinePoint;
if exists(select * from sysobjects where id=object_id('#tempWorkSitePass'))
 drop table #tempWorkSitePass;
if exists(select * from sysobjects where id=object_id('#detailPathLine'))
 drop table #detailPathLine;
end
Go

Create proc [dbo].[up_CoordinatesPointGoodsHistoryInfo]
(
		 @cardNumber int,
		 @cardType	 int,
		 @stime		datetime,
		 @etime		datetime
)
as
begin
	---轨迹详细信息
	declare  @tempPathLine table
	(
		cardNumber  nvarchar(50),
		goodsName  nvarchar(50),
		departName	nvarchar(50),
		worksiteAddress nvarchar(50),
		--areaName		nvarchar(50),
		worksiteid	int,
		STime		datetime,
		ETime		datetime,
		x			float,
		y			float,
		z			float
	)
set nocount on;
	---卡号轨迹
			select cardNum,worksiteid,STime,ETime,x,y,z into #pathLinePoint from worksitepass 
			where cardNum = @cardNumber and cardType = @cardType and stime >= @stime 
			 and stime <= @etime 

select * into #tempWorkSitePass from (
--- person轨迹
--insert into @tempPathLine convert(varchar(23),STime,120),convert(varchar(23),ETime,120)
select pLp.cardNum,a.name,dt.name as departName,
		ws.address,pLp.worksiteid,pLp.STime,pLp.ETime,pLp.x,pLp.y,pLp.z
from  #pathLinePoint pLp-- 一张卡的轨迹
inner join Articles a on pLp.cardNum=a.cardNumber
left join worksite ws on ws.worksiteid=pLp.worksiteid
LEFT JOIN dbo.department dt ON a.departmentId = dt.id 
where a.RegisterTime <= STime
union all
---> historyPerson 轨迹
select pLp.cardNum,ha.name,dt.name as departName,
		ws.address,pLp.worksiteid,pLp.STime,pLp.ETime,pLp.x,pLp.y,pLp.z
from  #pathLinePoint pLp-- 一张卡的轨迹
inner join HistoryArticles ha on pLp.cardNum=ha.cardNumber
left join worksite ws on ws.worksiteid=pLp.worksiteid
LEFT JOIN dbo.department dt ON ha.departmentId = dt.id
where ha.RegisterTime <= plp.Stime and ha.destoryTime >= plp.Stime
) t
order by stime asc
--p.intime < STime
--into #tempWorkSitePass
--union convert(varchar(23),occTime,120),convert(varchar(23),occTime,120)
select cardNum,worksiteid,cin.OccTime as STime,cin.OccTime as ETime,x,y,z into #detailPathLinePoint
 from CardLoc_Interval cin
where cardNum = @cardNumber and cardType = @cardType and cin.occTime >= @stime  
and cin.occTime <= @etime --order by cin.occTime asc

select * into #detailPathLine from 
(
select dpLp.cardNum,a.name,dt.name as departName,
		ws.address,dpLp.worksiteid,dpLp.STime,dpLp.ETime,dpLp.x,dpLp.y,dpLp.z
from  #detailPathLinePoint dpLp-- 一张卡的轨迹
inner join Articles a on dpLp.cardNum=a.cardNumber
left join worksite ws on ws.worksiteid=dpLp.worksiteid
LEFT JOIN dbo.department dt ON a.departmentId = dt.id
where a.RegisterTime <= STime
union all
---> historyPerson 轨迹
select dpLp.cardNum,ha.name,dt.name as departName,
		ws.address,dpLp.worksiteid,dpLp.STime,dpLp.ETime,dpLp.x,dpLp.y,dpLp.z
from  #detailPathLinePoint dpLp-- 一张卡的轨迹
inner join HistoryArticles ha on dpLp.cardNum=ha.cardNumber
left join worksite ws on ws.worksiteid=dpLp.worksiteid
LEFT JOIN dbo.department dt ON ha.departmentId = dt.id
where ha.RegisterTime <= dpLp.Stime and ha.destoryTime >= dpLp.Stime
)
t 
insert into @tempPathLine 
select * from #tempWorkSitePass union
select * from #detailPathLine 

select cardNumber,goodsName,departName,worksiteAddress,worksiteid,
convert(varchar(23),STime,120) as STime,convert(varchar(23),ETime,120) as ETime,x,y,z  
from @tempPathLine order by STime asc

--select * from #PathLine
if exists(select * from sysobjects where id=object_id('#pathLinePoint'))
 drop table #pathLinePoint;
if exists(select * from sysobjects where id=object_id('#tempWorkSitePass'))
 drop table #tempWorkSitePass;
if exists(select * from sysobjects where id=object_id('#detailPathLine'))
 drop table #detailPathLine;
end
Go

---区域树目录节点数目统计
Create procedure [dbo].[up_statisticsTreeChildNodeNum]
(
  @nodeString   nvarchar(100),  ---区域名
  @timeOut int,		---设置时间间隔 
  @currentUser varchar(50) 
)
as
declare @areaId int;
declare @ChildAreaList table
(	
	areaId   int
)
begin
set nocount on;
--if @currentUser = ''
-- set @currentUser = 'admin';
---人员查询
select * into #assaylocPerson from RealTimePersonList_Vw where datediff(s,occtime,getdate()) < @timeOut
--and departmentId in 
--(select udc.departId from userLogin ul left join userDepartConfig udc on ul.userName=udc.userName 
--  where ul.userName = @currentUser);
---物品查询
select * into #assaylocGoods from RealTimeGoodsList_Vw where datediff(s,occtime,getdate()) < @timeOut 
--and departmentId in 
--(select udc.departId from userLogin ul left join userDepartConfig udc on ul.userName=udc.userName 
--  where ul.userName = @currentUser);

select * into #assaylocVehicle from 
( SELECT   dbo.vehicle.id,dbo.vehicle.cardNumber,dbo.vehicle.Tag,
         dbo.Area.aName, ws.Address, cl.OccTime, ws.WorkSiteId,dbo.Area.id AS areaId
FROM     dbo.CardLoc AS cl INNER JOIN dbo.vehicle ON dbo.vehicle.cardNumber = cl.CardNum AND (cl.CardType = 3  or cl.CardType = 6)
			LEFT OUTER JOIN  dbo.WorkSite AS ws ON ws.WorkSiteId = cl.WorkSiteId LEFT OUTER JOIN
            dbo.corAreaWorksite AS caw ON ws.WorkSiteId = caw.WorksiteId LEFT OUTER JOIN
            dbo.Area ON dbo.Area.id = caw.areaId
) x
where datediff(s,occtime,getdate()) < @timeOut

--- 子区域数
    if exists(select id from Area where aName = @nodeString)		--- 查询区域id
		begin
			set @areaId = (select id from Area where aName = @nodeString);
			INSERT INTO @ChildAreaList EXEC up_getAreaUnderChildArea @areaId;
			select count(*)- 1 from @ChildAreaList;
		end
    else
	   begin
			select count(*) from @ChildAreaList;
	   end
	
--- 基站数
	select count(ws.worksiteid) from corAreaWorksite caw inner join worksite ws on caw.worksiteid=ws.worksiteid 
			 where caw.areaid in (select * from @ChildAreaList);
--- 人数
	select count(*) from #assaylocPerson rtperson where rtperson.areaid in (select * from @ChildAreaList);
--- 物品数
	select count(*) from #assaylocGoods  rtGoods where rtGoods.areaid in (select * from @ChildAreaList);
--- 车辆数
	select count(*) from #assaylocVehicle rtVehicle where rtVehicle.areaid in (select * from @ChildAreaList);

drop table #assaylocPerson
drop table #assaylocGoods
drop table #assaylocVehicle
end
Go

---部门树目录节点数目统计
Create procedure [dbo].[up_statisticsDepartTreeChildNodeNum]
(
  @nodeString   nvarchar(100),  ---部门名
  @timeOut int,		---设置时间间隔 
  @currentUser varchar(50) 
)
as
declare @departId int;
declare @ChildDepartList table
(	
	departId   int
)
begin
set nocount on;
--if @currentUser = ''
-- set @currentUser = 'admin';
---人员查询
select * into #assaylocPerson from RealTimeEmployeeList_Vw where datediff(s,occtime,getdate()) < @timeOut
--and departmentId in 
--(select udc.departId from userLogin ul left join userDepartConfig udc on ul.userName=udc.userName 
--  where ul.userName = @currentUser);
---物品查询
select * into #assaylocGoods from RealTimeGoodsList_Vw where datediff(s,occtime,getdate()) < @timeOut 
--and departmentId in 
--(select udc.departId from userLogin ul left join userDepartConfig udc on ul.userName=udc.userName 
--  where ul.userName = @currentUser);

--- 子部门数
    if exists(select id from department where name = @nodeString)		--- 查询部门id
		begin
			set @departId = (select id from department where name = @nodeString);
			INSERT INTO @ChildDepartList EXEC up_getDepartUnderChildDepart @departId;
			select count(*)- 1 from @ChildDepartList;
		end
    else
	   begin
			select count(*) from @ChildDepartList;
	   end

--- 人数
	select count(*) from #assaylocPerson rtperson where rtperson.departmentId in (select * from @ChildDepartList);
--- 物品数
	select count(*) from #assaylocGoods  rtGoods where rtGoods.departmentId in (select * from @ChildDepartList);
drop table #assaylocPerson
drop table #assaylocGoods
end
Go


-------------------------------------------2D新增存储过程--------------------------------------------------------

CREATE procedure [dbo].sp_AddUserRole(
@UserID uniqueidentifier,
@RoleID uniqueidentifier,
@return_error char(40) out
)
 AS
Begin 
  set @return_error = ''
  declare @count int
  select @count = count(*) from UserRole where UserID=@UserID
  if(@count<>0)
  begin
     Update UserRole Set RoleID =@RoleID where UserID = @UserID
  end
  else
  begin
     Insert into UserRole(UserID,RoleID) values(@UserID,@RoleID)
  end 
  if (@@error<>0)
    begin
      /*回滚事务由外部处理*/
      set @return_error = '增加角色出错'
      return 
    end
End
GO
-------------------------------------------------------------------------------------------------------

------------------------------sp_copyrolepermission----------------------------------------------------

CREATE procedure [dbo].[sp_copyrolepermission](
@fromroleid uniqueidentifier,
@toroleid uniqueidentifier,
@return_error char(40) out
)
 AS
Begin 
  --模块标识
  declare @moduleid uniqueidentifier
  --操作标识
  declare @operateid uniqueidentifier
  --定义游标
  declare rolepermission_cursor cursor for select MenuListID,OperateID from rolepermission where roleid=@fromroleid
  --清除目标角色权限
  delete rolepermission where roleid=@toroleid

  --设置异常变量初始值
  set @return_error = ''
  
  --打开游标,准备复制权限
  open rolepermission_cursor
  fetch next from rolepermission_cursor into @moduleid,@operateid
  while @@fetch_status=0
  begin
    insert into rolepermission(ID, RoleID, MenuListID, OperateID) values(newid(), @toroleid, @moduleid, @operateid)
    if (@@error<>0)
    begin
      /*回滚事务由外部处理*/
      set @return_error = '复制权限发生错误'
      return 
    end
    fetch next from rolepermission_cursor into @moduleid,@operateid
  end
  close rolepermission_cursor
  deallocate rolepermission_cursor
End
GO


---------------------------------------------------------------------------------------------------

------------------------------------
CREATE procedure [dbo].[sp_addrolepermission](
	@RoleID uniqueidentifier,
	@MenuListID uniqueidentifier,
	@OperateID uniqueidentifier,
	@return_error varchar(40) out
)
 AS
Begin 
  --设置异常变量初始值
  set @return_error = ''
  
  insert into rolepermission(ID, RoleID, MenuListID, OperateID) values(newid(), @RoleID, @MenuListID, @OperateID)
  if (@@rowcount = 0)
  begin
      set  @return_error = '增加角色权限失败，影响行数为0行'
      return
  end
End
GO


-------------------------------------------------------------------------------------------------------------------

CREATE procedure [dbo].[sp_copyuserrole](
@fromuserid uniqueidentifier,
@touserid uniqueidentifier,
@return_error char(40) out
)
 AS
Begin 
  --角色标识
  declare @roleid uniqueidentifier
  --定义游标
  declare userrole_cursor cursor for select RoleID from UserRole where userid=@fromuserid
  --清除目标用户角色
  delete UserRole where userid=@touserid

  --设置异常变量初始值
  set @return_error = ''
  
  --打开游标,准备复制权限
  open userrole_cursor
  fetch next from userrole_cursor into @roleid
  while @@fetch_status=0
  begin
    insert into UserRole(UserID, RoleID) values(@touserid, @roleid)
    if (@@error<>0)
    begin
      /*回滚事务由外部处理*/
      set @return_error = '复制角色时发生错误'
      return 
    end
    fetch next from userrole_cursor into @roleid
  end
  close userrole_cursor
  deallocate userrole_cursor
End

GO

---------------------------------------------------------------------------------------

-------------------------------------加载角色权限--------------------------------------
create Procedure [dbo].[sp_GetRolePermission]
(
   @gRoleID uniqueidentifier,
   @ParentID uniqueidentifier
)
As
Begin
  --定义最终查询语句
  DECLARE @selectSQL nvarchar(3000)
  set @selectSQL = 'create table #RolePermission
                      (RoleID uniqueidentifier,
					   MenulistID uniqueidentifier,'
   --获取操作的列名作为临时表的列
   DECLARE @ColumnNames VARCHAR(1000)
    SET @ColumnNames=''
    SELECT
       @ColumnNames = @ColumnNames + '' + Name + ' bit DEFAULT 0,'
    FROM
       (SELECT DISTINCT Name FROM Operate) t
    SET @ColumnNames= LEFT(@ColumnNames, LEN(@ColumnNames)-1)
	--拼装临时表
	set @selectSQL = @selectSQL + @ColumnNames+')'
	--当父级ID为00000000-0000-0000-0000-000000000000时及代表管理员权限，需要将所有功能菜单加载(IsBizModule = 1)
	if(@ParentID = '00000000-0000-0000-0000-000000000000')
	begin
	  set @selectSQL= @selectSQL + ' insert into #RolePermission(MenuListID) 
	                               select DISTINCT ID from menulist where  isBizModule = 1 
								   Update #RolePermission set RoleID = '''+CAST(@gRoleID AS varchar(200))+'''
								   '
	end
	else
	begin
	set @selectSQL= @selectSQL + ' insert into #RolePermission(MenuListID)
	                               select DISTINCT MenuListID from RolePermission_View where RoleID in ('''+CAST(@ParentID AS varchar(200))+''','''+CAST(@gRoleID AS varchar(200))+''')  
								   Update #RolePermission set RoleID = '''+CAST(@gRoleID AS varchar(200))+'''
								   '
    end

    set @selectSQL = @selectSQL + '
	declare @OperateName varchar(50)
    declare @MenuListID uniqueidentifier 
   declare @RoleID uniqueidentifier 
   --定义游标
   declare OperateName_cursor cursor for select RoleID,MenuListID,OperateName from RolePermission_View where RoleID in ('''+CAST(@ParentID AS varchar(200))+''','''+CAST(@gRoleID AS varchar(200))+''')  
   --打开游标
   open OperateName_cursor
   fetch next from OperateName_cursor into @RoleID,@MenuListID,@OperateName
   while @@fetch_status=0
	  begin
		declare   @s   varchar(1000)     
		set @s =''update #RolePermission set '' +@operateName+'' = 1 where RoleID =''''''+CAST(@RoleID AS varchar(200))+'''''' and MenuListID =''''''+CAST(@MenuListID AS varchar(200))+''''''''
		Exec(@s)
		fetch next from OperateName_cursor into @RoleID,@MenuListID,@OperateName
	  end
  close OperateName_cursor
  deallocate OperateName_cursor 
	'
	
			
	set @selectSQL =@selectSQL+' select m.Name as 模块名称,m.OperateListID,m.PageID,rp.* from #RolePermission rp 
	                             left join MenuList m on rp.MenuListID = m.ID where Disabled =0 and roleID = '''+CAST(@gRoleID AS varchar(200))+''' order by PageID asc'
    set @selectSQL = @selectSQL +' drop table #RolePermission'
	exec sp_executesql @selectSQL
End

GO

---------------------------------------------------------------------------------------

-------------------------------------加载用户菜单--------------------------------------

Create procedure sp_GetMenu
(@userID uniqueidentifier,
 @OperateName varchar(50),
 @return_error char(40) out
)
AS
Begin
 Declare @MenuList table
 (
       ID uniqueidentifier,    
	   Name varchar(20),                    
	   ParentID uniqueidentifier,             
	   SerialNum int,                       
	   IsBizModule bit,                     
	   NameSpace varchar(100),                 
	   ViewName varchar(50),    
	   IconResourceName varchar(50),
	   HIconResourceName  varchar(50),               
	   Disabled bit,
	   Description varchar(30)  , 
	   PageID int ,
	   OperateListID varchar(1000)               
 )
   --先查出当前用户的具有查询权限的菜单
 INSERT  INTO @MenuList 
             Select  M.*
			 From    UserPermission_View up
			 Left join MenuList M on M.Name = up.ModuleName
			 Where up.UserID=@userID and up.OperateName=@OperateName and M.Disabled = 0
  --查处IsBizModule==0的菜单，这类菜单通常为非功能菜单，判断其下是否有子菜单，如果有插入到@MenuList中
  declare @BizMenuListID uniqueidentifier
   --定义游标
  declare BizMenuList_cursor cursor for select ID from MenuList where IsBizModule=0 and Disabled=0
   --设置异常变量初始值
  set @return_error = ''
   --打开游标
   open BizMenuList_cursor
   fetch next from BizMenuList_cursor into @BizMenuListID
   while @@fetch_status=0
  begin
    declare @count int
	select @count=count(*) from @MenuList where ParentID=@BizMenuListID and Disabled=0
	if(@count<>0)
	begin
	   insert into @MenuList
	   select * from MenuList
	   where id = @BizMenuListID
	end
    if (@@error<>0)
    begin
      /*回滚事务由外部处理*/
      set @return_error = '获取菜单失败'
      return 
    end
    fetch next from BizMenuList_cursor into @BizMenuListID
  end
  close BizMenuList_cursor
  deallocate BizMenuList_cursor
 
 SELECT a.*, FatherMenuList.Name AS ParentName
FROM @MenuList a LEFT OUTER JOIN
      dbo.MenuList FatherMenuList ON a.ParentID = FatherMenuList.ID
End
GO

---------------------------------------------------------------------------------------
------------------------新增判断报警图标-----------------------------------------


CREATE PROCEDURE [dbo].[sp_GetAlarmTable]
    (
      @NoSignAuto INT , --无信号是否自动消警 1 自动  0 非自动
      @FaultAuto INT , --故障是否自动消警 1 自动  0 非自动
      @Time INT --超时时间
    )
AS 
    BEGIN
      select COUNT(id) as AreaAccessAlarmCount from AreaAlarmMsg_Vw_2D with (nolock)
        WHERE   isread = 0--区域准入报警
        
      select COUNT(id) as AutoCardLowPowerAlarmCount from 
      CardLowPower where isRead = 0  and (CardType=3) ---定位卡低电报警
      
       select COUNT(id) as WristLowPowerAlarmCount from 
      CardLowPower where isRead = 0  and (CardType= 6) ---腕带低电报警
      
       select COUNT(id) as PagerLowPowerAlarmCount from 
      CardLowPower where isRead = 0  and (CardType=9) ---呼叫器低电报警
      
       select COUNT(id) as ChestLowPowerAlarmCount from 
      CardLowPower where isRead = 0  and (CardType=10) ---胸牌低电报警
        
       select COUNT(id) AreaOverTimeAlarmCount  from AreaOverTimeAlarmMsg_Vw_2D with (nolock)
       where IsRead=0  --区域超时报警
       
       select COUNT(id) as SOSAlarmCount  from cardSosAlarmMsg_Vw_2D with (nolock)
       where isread=0  --求救报警
       
       if(@NoSignAuto=1)
           begin
          select COUNT(id)  as NoSignAlarmCount  from NoSignal_Person_AlarmMsg_Vw_2D  with (nolock)
           where isRead = 0 and OKTime is  null
           end
        else
           begin
            select COUNT(id)  as NoSignAlarmCount  from NoSignal_Person_AlarmMsg_Vw_2D  with (nolock)
            where isRead = 0 
           end   ---无信号报警
           
         if(@FaultAuto=1)
           begin
           SELECT   count(dbo.WorkSiteAlarm.id) as WorkSiteFaultAlarmCount FROM    dbo.WorkSiteAlarm LEFT JOIN
           dbo.WorkSite ON dbo.WorkSiteAlarm.WorkSiteId = dbo.WorkSite.WorkSiteId
           WHERE     (dbo.WorkSiteAlarm.WorkSiteId >= 1000) AND (dbo.WorkSiteAlarm.WorkSiteId < 65535) and isRead = 0 and OKTime is  null
           end 
         else
           begin
               
           SELECT   count(dbo.WorkSiteAlarm.id) as WorkSiteFaultAlarmCount FROM    dbo.WorkSiteAlarm LEFT JOIN
           dbo.WorkSite ON dbo.WorkSiteAlarm.WorkSiteId = dbo.WorkSite.WorkSiteId
           WHERE     (dbo.WorkSiteAlarm.WorkSiteId >= 1000) AND (dbo.WorkSiteAlarm.WorkSiteId < 65535) and isRead = 0 
           end    --基站故障报警
           
           
          if(@FaultAuto=1)
           begin
           SELECT   count(dbo.WorkSiteAlarm.id) as WorkSiteUnitFaultAlarmCount FROM    dbo.WorkSiteAlarm LEFT JOIN
           dbo.WorkSite ON dbo.WorkSiteAlarm.WorkSiteId = dbo.WorkSite.WorkSiteId
           WHERE    (dbo.WorkSiteAlarm.WorkSiteId < 1000) and isRead = 0 and OKTime is  null
           end 
         else
           begin
           SELECT   count(dbo.WorkSiteAlarm.id) as WorkSiteUnitFaultAlarmCount FROM    dbo.WorkSiteAlarm LEFT JOIN
           dbo.WorkSite ON dbo.WorkSiteAlarm.WorkSiteId = dbo.WorkSite.WorkSiteId
           WHERE     (dbo.WorkSiteAlarm.WorkSiteId < 1000) and isRead = 0 
           end    --基站控制单元故障报警
           
           
             if(@FaultAuto=1)
           begin
           SELECT   count(id) as MattressFaultAlarmCount FROM   CardException
           WHERE    CardType=8 and isRead = 0 and OKTime is  null
           end 
         else
           begin
           SELECT   count(id) as MattressFaultAlarmCount FROM   CardException
           WHERE    CardType=8 and isRead = 0 
           end    --智能床垫故障报警
           
           
             if(@FaultAuto=1)
           begin
           SELECT   count(id) as PagerFaultAlarmCount FROM   CardException
           WHERE    CardType=9 and isRead = 0 and OKTime is  null
           end 
         else
           begin
           SELECT   count(id) as PagerFaultAlarmCount FROM   CardException
           WHERE    CardType=9 and isRead = 0 
           end    --呼叫器故障报警
           
           
           select COUNT(id) as TumbleAlarmCount  from WristFallAlarmView_2D  with (nolock)
            where isRead = 0  --跌倒报警
            
          select COUNT(id)  as AbnormalMotionlessAlarmCount from WristCardSilentAlarmView_2D  with (nolock)
           where IsRead=0   --异常静止报警
           
          select COUNT(id)  as DisassemblyAlarmCount  from  DisassemblyAlarmView_2D  with (nolock)
           where IsRead=0   --拆卸报警
           
           select COUNT(id) as PagerCallAlarmCount  from  pagerCallAlarmMsg_View_2D with (nolock)
            where IsRead=0  --呼叫报警
           select COUNT(id)  as  MattressCallAlarmCount  from MattressAlarmMsg_View_2D  with  (nolock)
            where IsRead=0  -- 离床报警
           select  distinct(CardNum) from CardSos  where IsRead=0
             -- 求救报警 报警人工号
    END


GO
----------------------------判断基站是否异常-----------------------------
create procedure SP_iSWorkSiteAlarmByID
(
   @worksiteID int,
   @return bit out --是否异常，0正常，1异常
)
as
declare @minTime varchar(50)
declare @CardAbnormalTimeSpan varchar(50)
select @minTime = DATEDIFF(minute, OccTime, GETDATE()) from WorkSite where WorkSiteId = @worksiteID
select @CardAbnormalTimeSpan = Value from Settings where Item = 'WorksiteAbnormalTimeSpan'

if(@minTime IS NOT NULL and @CardAbnormalTimeSpan IS NOT NULL and Convert(float,@minTime) <  Convert(float,@CardAbnormalTimeSpan))
begin
  set @return = 0;
  return;
end
else
begin
   set @return =1;
   return;
end

go
----------------------------------------------------------------------------------------------


CREATE FUNCTION [dbo].[fu_IsNull] ( @value VARCHAR(50) )
RETURNS INT
AS 
    BEGIN
        
        IF @value IS NULL 
            BEGIN
                RETURN 0
            END
        ELSE 
            IF @value = '' 
                BEGIN
                    RETURN 0
                END
            ELSE 
                BEGIN
                    RETURN 1
                END
        
        RETURN 0
    END

	GO

CREATE proc [dbo].[sp_AddUpEmpNum]
(
    @timespanSec int		---设置时间间隔
)
AS
Begin
   Declare @base Table
   (
      CardNum varchar(100),
	  CardType int,
	  WorkSiteId int,
	  departmentId int,
	  x float,
	  y float,
	  occTime datetime
   )
   insert into @base
   (
      CardNum,
	  CardType,
	  WorkSiteId,
	  departmentId,
	  x,
	  y,
	  occTime
   )
   select * 
   from (
      --查找人员 CardType=3
	  select a.CardNum,
	         a.CardType,
			 a.WorkSiteId,
			 empvw.departmentId,
			 a.x,
			 a.y,
			 a.OccTime
	   from CardLoc_2D a
	   INNER JOIN Person as empvw on empvw.cardNumber = a.CardNum and (a.CardType = 3 or a.CardType = 6 or a.CardType=10)
	   where DATEDIFF(SECOND,occtime,GETDATE()) < @timespanSec
	   
	   UNION ALL
	   --查找设备 CardType=4
	   select a.CardNum,
	          a.CardType,
			  a.WorkSiteId,
			  articles.departmentId,
			  a.x,
			  a.y,
			  a.OccTime
	   from CardLoc_2D a
       inner join articles on articles.cardNumber= a.cardNum and a.CardType = 4
	   where DATEDIFF(SECOND,a.OccTime,GETDATE()) < @timespanSec
   ) as att;
   
   select * from @base

   ---------------------group -----------------------------------
--group by department

--   SELECT  dep.id ,
--            ISNULL(dep.name, '无部门') AS name ,
--            dep.number ,
--            dep.parentId ,
--            ISNULL(t1.sum_department,0) As sum_department
--    FROM    ( SELECT    COUNT(*) AS sum_department ,
--                        e.id
--              FROM      @base s
--                        left join department e ON s.departmentId = e.id
--              WHERE     s.cardType=3
--              GROUP BY  e.id
--            ) AS t1
--          full outer join  department AS dep ON dep.id = t1.id ORDER BY t1.sum_department DESC 

----group by area


--    SELECT  area.id ,
--            area.number ,
--            area.name ,
--            area.parentId ,
--            area.typeId ,
--            ISNULL(t1.sum_area,0) as sum_area,
--            area.isAllow,
--			area.outSpanTime,
--			area.personSize
--    FROM    ( SELECT    COUNT(*) AS sum_area ,
--                        wa.areaId AS areaId
--              FROM      @base s
--                        INNER JOIN worksite e ON s.WorkSiteId = e.WorkSiteId
--                        INNER JOIN corAreaWorkSite wa ON e.WorkSiteId = wa.WorksiteId
--              WHERE     s.cardType=4
--              GROUP BY  wa.areaId
--            ) AS t1
--            RIGHT JOIN area on area.id=t1.areaId
			
End


GO


-----------------------------------SP_Page---------------------------------------------
CREATE PROCEDURE [dbo].[sp_page]
    (
      @psTblName VARCHAR(MAX) = NULL , -- 设置表名
      @psStrGetFields VARCHAR(MAX) = '*' ,  -- 设置输出字段名
      @psFieldName VARCHAR(MAX) = NULL , -- 设置排序字段名（只能设1个字段）
      @psPageSize INT = 10 ,   -- 设置分页尺寸，大于0
      @psPageIndex INT = 1 ,    -- 设置分页码，大于0
      @psOrderType BIT = 0 ,    -- 排序类型，非0值则降序
      @psStrWhere VARCHAR(MAX) = NULL , -- 设置条件，不加where
      @rsPageCount INT = 0 OUTPUT , -- 输出页数
      @rsRecordCount INT = 0 OUTPUT  -- 输出记录数
    )
AS 
    BEGIN
        SET nocount ON
    -- 参数合法检查
        IF ( dbo.fu_isnull(@psTblName) = 0
             OR dbo.fu_isnull(@psFieldName) = 0
             OR dbo.fu_isnull(@psPageIndex) = 0
             OR dbo.fu_isnull(@psPageSize) = 0
             OR @psPageIndex <= 0
             OR @psPageSize <= 0
           ) 
            BEGIN
                RETURN 0
            END
    -- 判断返回列名字段为空时设置为'*'
        IF ( dbo.fu_isnull(@psStrGetFields) = 0 ) 
            BEGIN
                SET @psStrGetFields = '*'
            END
    -- 定义参数
        DECLARE @strNsql NVARCHAR(MAX) -- 主语句
        DECLARE @strSql VARCHAR(MAX)  -- 主语句
        DECLARE @strMax VARCHAR(MAX)   -- 求Max()语句
        DECLARE @strFlag VARCHAR(MAX)    -- 标致值'<','>'
        DECLARE @strParams VARCHAR(MAX)   -- 参数值 
        DECLARE @strOrder VARCHAR(MAX)   -- 排序类型
        DECLARE @strPageSize VARCHAR(MAX)    -- 页大小
        DECLARE @strDropTbls VARCHAR(MAX)   -- 删除临时数
    -- 计算总记录数(OUTPUT参数)
        IF ( dbo.fu_isnull(@psStrWhere) = 1 ) 
            BEGIN
                SET @strNsql = ' select @rsRecordCount=count(1) ' + ' from '
                    + @psTblName + ' where ' + @psStrWhere
            END
        ELSE 
            BEGIN
                SET @strNsql = ' select @rsRecordCount=count(1) ' + ' from '
                    + @psTblName
            END
    -- 执行SQL语句，求出总记录数
        EXEC sp_executesql @strNsql, N'@rsRecordCount int output',
            @rsRecordCount OUTPUT
        IF @@error <> 0 
            BEGIN
                RETURN 0
            END
    -- 判断Count('*')总数是否为0
        IF ( @rsRecordCount <= 0 ) 
            BEGIN
                SET @rsPageCount = 0
                EXEC ('select '+@psStrGetFields+' from '+@psTblName+' where 1=2')
                IF @@error <> 0 
                    BEGIN
                        RETURN 0
                    END
                ELSE 
                    BEGIN
                        RETURN 1
                    END            
            END
        ELSE 
            BEGIN
        -- 计算总页数（output参数）
                SET @rsPageCount = CEILING(@rsRecordCount * 1.0 / @psPageSize)
            END
 -- 把页大小转换为字符串
        SET @strPageSize = STR(@psPageSize)
 
 -- 分页码大于总页数，则把分页码置为总页数，表示取最后一页
        IF ( @psPageIndex > @rsPageCount
             AND @rsPageCount > 0
           ) 
            BEGIN
                SET @psPageIndex = @rsPageCount
            END
 
 -- 如果@psOrderType不是0，则降序
        IF ( @psOrderType <> 0 ) 
            BEGIN
                SET @strFlag = '<'
                SET @strParams = 'min'
                SET @strOrder = ' order by ' + @psFieldName + ' desc '
            END
        ELSE 
            BEGIN
                SET @strFlag = '>'
                SET @strParams = 'max'
                SET @strOrder = ' order by ' + @psFieldName + ' asc '
            END
 
 -- 主语句（如果是第一次则加速执行）
        IF ( @psPageIndex = 1 ) 
            BEGIN
                IF ( dbo.fu_isnull(@psStrWhere) = 1 ) 
                    BEGIN
                        SET @strSql = ' select top '
                            + LTRIM(RTRIM(STR(@psPageSize))) + ' '
                            + @psStrGetFields + ' from ' + @psTblName
                            + ' where '
                    END
                ELSE 
                    BEGIN
                        SET @strSql = ' select top '
                            + LTRIM(RTRIM(STR(@psPageSize))) + ' '
                            + @psStrGetFields + ' from ' + @psTblName
                    END
            END
        ELSE 
            BEGIN
     -- 如果TOP值少于5300，把数据插入临时表，求MAX值
                IF ( ( @psPageIndex - 1 ) * @psPageSize ) <= 5300 
                    BEGIN
                        SET @strMax = ' select top '
                            + LTRIM(RTRIM(STR(( @psPageIndex - 1 )
                                              * @psPageSize))) + ' '
                            + @psStrGetFields + ' into #Tbls from '
                            + @psTblName
         
                        SET @strDropTbls = ' drop table #Tbls'
 
         -- 判断是否有条件值
                        IF ( dbo.fu_isnull(@psStrWhere) = 1 ) 
                            BEGIN
                                SET @strMax = @strMax + ' where '
                                    + @psStrWhere + ' ' + @strOrder
 
                                SET @strSql = 'select top '
                                    + LTRIM(RTRIM(STR(@strPageSize))) + ' '
                                    + @psStrGetFields + ' from ' + @psTblName
                                    + ' where ' + @psFieldName + '' + @strFlag
                                    + '(select ' + @strParams + '('
                                    + @psFieldName + ') from #Tbls) and '
                            END
                        ELSE 
                            BEGIN
                                SET @strMax = @strMax + @strOrder
                                SET @strSql = 'select top '
                                    + LTRIM(RTRIM(STR(@strPageSize))) + ' '
                                    + @psStrGetFields + ' from ' + @psTblName
                                    + ' where ' + @psFieldName + '' + @strFlag
                                    + '(select ' + @strParams + '('
                                    + @psFieldName + ') from #Tbls) '
                            END
                    END
                ELSE 
                    BEGIN
         -- 不用临时表来求SQL语句
                        DECLARE @xtype INT
                        SELECT  @xtype = xtype
                        FROM    syscolumns
                        WHERE   id = OBJECT_ID('' + @psTblName + '')
                                AND name = @psFieldName
                        IF ( @xtype = 61 ) 
                            BEGIN
                                SET @strMax = 'declare @strTmp varchar(500)'
                                    + ' select @strTmp= CONVERT(varchar(30),'
                                    + @strParams + '(' + @psFieldName
                                    + ') ,109)  from (select top '
                                    + LTRIM(RTRIM(STR(( @psPageIndex - 1 )
                                                      * @psPageSize))) + ' '
                                    + @psFieldName + ' from ' + @psTblName
                            END
                        ELSE 
                            BEGIN
                                SET @strMax = 'declare @strTmp varchar(500)'
                                    + ' select @strTmp= ' + @strParams + '('
                                    + @psFieldName + ') from (select top '
                                    + LTRIM(RTRIM(STR(( @psPageIndex - 1 )
                                                      * @psPageSize))) + ' '
                                    + @psFieldName + ' from ' + @psTblName
                            END
        
         
         -- 判断是否有条件值
                        IF ( dbo.fu_isnull(@psStrWhere) = 1 ) 
                            BEGIN
                                SET @strSql = ' where ' + @psStrWhere + ' '
                                    + @strOrder + ') as tblTmp select top '
                                    + @strPageSize + ' ' + @psStrGetFields
                                    + ' from ' + @psTblName + ' where '
                                    + @psFieldName + ' ' + @strFlag
                                    + ' @strTmp and '
                            END
                        ELSE 
                            BEGIN
                                SET @strSql = @strOrder
                                    + ') as tblTmp select top ' + @strPageSize
                                    + ' ' + @psStrGetFields + ' from '
                                    + @psTblName + ' where ' + @psFieldName
                                    + ' ' + @strFlag + ' @strTmp '
                            END
                    END
            END
    -- print @strMax
    -- print @strSql
    -- print @psStrWhere
    -- print @strOrder
    -- print @strDropTbls
    -- 输出

        EXEC (@strMax+@strSql+@psStrWhere+@strOrder+@strDropTbls)
      
        IF @@error <> 0 
            BEGIN
                RETURN 0
            END
        ELSE 
            BEGIN
                RETURN 1
            END
    END

/*################################################*/

    SET QUOTED_IDENTIFIER ON
    SET ANSI_NULLS ON


/*#####################################跳传链路信息存储过程##########################################################*/

GO

create procedure [dbo].[SP_InsertWifiLinkInfo] (
 	@CurrentID INT,
    @ParentID Int,
 	@OccTime DATETIME
 ) as
BEGIN
	SET NOCOUNT ON;
	
	INSERT INTO dbo.WifiLinkHistory(CurrentID, ParentID, OccTime) VALUES(@CurrentID, @ParentID, @OccTime)
	
	IF EXISTS(SELECT * FROM dbo.WifiLinkInfo WHERE CurrentID = @CurrentID)
		UPDATE dbo.WifiLinkInfo SET ParentID = @ParentID, OccTime = @OccTime WHERE CurrentID = @CurrentID
	ELSE
		INSERT INTO dbo.WifiLinkInfo(CurrentID,ParentID, OccTime) VALUES(@CurrentID, @ParentID,@OccTime)
END


GO

/*#####################################跳传链路结束##########################################################*/

/*################################################*/




------------------------------------------------------------------------------------------------------------------

-----------------------------------------Procedure End-------------------------------------------------------------
GO
-------------------------------------------InitData Begin---------------------------------------------------------

--初始数据
--RssiWeight
insert into RssiWeight(rssi,weight) values(-100,50);
insert into RssiWeight(rssi,weight) values(-99,56);
insert into RssiWeight(rssi,weight) values(-98,62);
insert into RssiWeight(rssi,weight) values(-97,70);
insert into RssiWeight(rssi,weight) values(-96,79);
insert into RssiWeight(rssi,weight) values(-95,88);
insert into RssiWeight(rssi,weight) values(-94,99);
insert into RssiWeight(rssi,weight) values(-93,111);
insert into RssiWeight(rssi,weight) values(-92,125);
insert into RssiWeight(rssi,weight) values(-91,140);
insert into RssiWeight(rssi,weight) values(-90,158);
insert into RssiWeight(rssi,weight) values(-89,177);
insert into RssiWeight(rssi,weight) values(-88,199);
insert into RssiWeight(rssi,weight) values(-87,223);
insert into RssiWeight(rssi,weight) values(-86,250);
insert into RssiWeight(rssi,weight) values(-85,281);
insert into RssiWeight(rssi,weight) values(-84,315);
insert into RssiWeight(rssi,weight) values(-83,353);
insert into RssiWeight(rssi,weight) values(-82,397);
insert into RssiWeight(rssi,weight) values(-81,445);
insert into RssiWeight(rssi,weight) values(-80,500);
insert into RssiWeight(rssi,weight) values(-79,561);
insert into RssiWeight(rssi,weight) values(-78,629);
insert into RssiWeight(rssi,weight) values(-77,706);
insert into RssiWeight(rssi,weight) values(-76,792);
insert into RssiWeight(rssi,weight) values(-75,889);
insert into RssiWeight(rssi,weight) values(-74,997);
insert into RssiWeight(rssi,weight) values(-73,1119);
insert into RssiWeight(rssi,weight) values(-72,1255);
insert into RssiWeight(rssi,weight) values(-71,1409);
insert into RssiWeight(rssi,weight) values(-70,1581);
insert into RssiWeight(rssi,weight) values(-69,1774);
insert into RssiWeight(rssi,weight) values(-68,1990);
insert into RssiWeight(rssi,weight) values(-67,2233);
insert into RssiWeight(rssi,weight) values(-66,2505);
insert into RssiWeight(rssi,weight) values(-65,2811);
insert into RssiWeight(rssi,weight) values(-64,3154);
insert into RssiWeight(rssi,weight) values(-63,3539);
insert into RssiWeight(rssi,weight) values(-62,3971);
insert into RssiWeight(rssi,weight) values(-61,4456);
insert into RssiWeight(rssi,weight) values(-60,5000);
insert into RssiWeight(rssi,weight) values(-59,5610);
insert into RssiWeight(rssi,weight) values(-58,6294);
insert into RssiWeight(rssi,weight) values(-57,7062);
insert into RssiWeight(rssi,weight) values(-56,7924);
insert into RssiWeight(rssi,weight) values(-55,8891);
insert into RssiWeight(rssi,weight) values(-54,9976);
insert into RssiWeight(rssi,weight) values(-53,11193);
insert into RssiWeight(rssi,weight) values(-52,12559);
insert into RssiWeight(rssi,weight) values(-51,14091);
insert into RssiWeight(rssi,weight) values(-50,15811);
insert into RssiWeight(rssi,weight) values(-49,17740);
insert into RssiWeight(rssi,weight) values(-48,19905);
insert into RssiWeight(rssi,weight) values(-47,22334);
insert into RssiWeight(rssi,weight) values(-46,25059);
insert into RssiWeight(rssi,weight) values(-45,28117);
insert into RssiWeight(rssi,weight) values(-44,31547);
insert into RssiWeight(rssi,weight) values(-43,35397);
insert into RssiWeight(rssi,weight) values(-42,39716);
insert into RssiWeight(rssi,weight) values(-41,44562);
insert into RssiWeight(rssi,weight) values(-40,50000);
insert into RssiWeight(rssi,weight) values(-39,56100);
insert into RssiWeight(rssi,weight) values(-38,62946);
insert into RssiWeight(rssi,weight) values(-37,70626);
insert into RssiWeight(rssi,weight) values(-36,79244);
insert into RssiWeight(rssi,weight) values(-35,88913);
insert into RssiWeight(rssi,weight) values(-34,99763);
insert into RssiWeight(rssi,weight) values(-33,111936);
insert into RssiWeight(rssi,weight) values(-32,125594);
insert into RssiWeight(rssi,weight) values(-31,140919);
insert into RssiWeight(rssi,weight) values(-30,158113);
insert into RssiWeight(rssi,weight) values(-29,177406);
insert into RssiWeight(rssi,weight) values(-28,199053);
insert into RssiWeight(rssi,weight) values(-27,223341);
insert into RssiWeight(rssi,weight) values(-26,250593);
insert into RssiWeight(rssi,weight) values(-25,281170);
insert into RssiWeight(rssi,weight) values(-24,315478);
insert into RssiWeight(rssi,weight) values(-23,353972);
insert into RssiWeight(rssi,weight) values(-22,397164);
insert into RssiWeight(rssi,weight) values(-21,445625);
insert into RssiWeight(rssi,weight) values(-20,500000);
insert into RssiWeight(rssi,weight) values(-19,1000000);
insert into RssiWeight(rssi,weight) values(-18,1000000);
insert into RssiWeight(rssi,weight) values(-17,1000000);
insert into RssiWeight(rssi,weight) values(-16,1000000);
insert into RssiWeight(rssi,weight) values(-15,1000000);
insert into RssiWeight(rssi,weight) values(-14,1000000);
insert into RssiWeight(rssi,weight) values(-13,1000000);
insert into RssiWeight(rssi,weight) values(-12,1000000);
insert into RssiWeight(rssi,weight) values(-11,1000000);
insert into RssiWeight(rssi,weight) values(-10,1000000);
insert into RssiWeight(rssi,weight) values(-9,1000000);
insert into RssiWeight(rssi,weight) values(-8,1000000);
insert into RssiWeight(rssi,weight) values(-7,1000000);
insert into RssiWeight(rssi,weight) values(-6,1000000);
insert into RssiWeight(rssi,weight) values(-5,1000000);
insert into RssiWeight(rssi,weight) values(-4,1000000);
insert into RssiWeight(rssi,weight) values(-3,1000000);
insert into RssiWeight(rssi,weight) values(-2,1000000);
insert into RssiWeight(rssi,weight) values(-1,1000000);
GO

---MattressAlarmSet
insert into MattressAlarmSet(StartTime,EndTime,Status) values('1900-01-01 19:00:00.000','1900-01-01 23:59:59.599',0)
insert into MattressAlarmSet(StartTime,EndTime,Status) values('1900-01-01 00:00:00.000','1900-01-01 07:00:00.000',0)

--ArticlesStatus
insert into ArticlesStatus(StatusId,StatusName) Values(0,'良好');
insert into ArticlesStatus(StatusId,StatusName) Values(1,'待检测');
insert into ArticlesStatus(StatusId,StatusName) Values(2,'待维修');
insert into ArticlesStatus(StatusId,StatusName) Values(3,'报废');
GO

--ArticleCardType
insert into ArticleCardType(typeId,typeName) Values(1,'按卡号查找')
insert into ArticleCardType(typeId,typeName) Values(2,'按卡类型查找')
insert into ArticleCardType(typeId,typeName) Values(3,'取消查找')
insert into ArticleCardType(typeId,typeName) Values(4,'下发卡设置')
GO

--CardTypeManager
insert into CardTypeManager(id,Name) select 1,'人员卡'
insert into CardTypeManager(id,Name) select 2,'设备卡'
insert into	CardTypeManager(id,Name) select 3,'人员设备卡'
GO
--userLogin
INSERT INTO userLogin(userName,password,trueName,lastLoginTime,createDateTime,permissionId) 
VALUES('admin','123456','超级管理员','',getdate(),1);
GO


--AlarmType
insert  AlarmType(AlarmID,AlarmName) VALUES(1,'求救报警') 
insert  AlarmType(AlarmID,AlarmName) VALUES (7,'无信号报警')
insert 	AlarmType(AlarmID,AlarmName) VALUES (8,'卡低电报警')
insert  AlarmType(AlarmID,AlarmName) VALUES (11,'区域准入报警')
insert  AlarmType(AlarmID,AlarmName) VALUES (12,'区域超时报警')
insert 	AlarmType(AlarmID,AlarmName) VALUES (10,'故障报警')
insert  AlarmType(AlarmID,AlarmName) VALUES (4,'跌倒报警')
insert  AlarmType(AlarmID,AlarmName) VALUES (6,'异常静止报警')
insert 	AlarmType(AlarmID,AlarmName) VALUES (5,'拆卸报警')
insert 	AlarmType(AlarmID,AlarmName) VALUES (2,'呼叫报警')
insert 	AlarmType(AlarmID,AlarmName) VALUES (3,'离床报警')
insert 	AlarmType(AlarmID,AlarmName) VALUES (9,'呼叫器低电报警')
GO

--Role
INSERT dbo.Role(id,RoleName, RoleDescription,parentRoleID,CardTypeManagerId) VALUES(1,'超级管理员','人员和设备管理',null,3)
INSERT dbo.Role(id,RoleName, RoleDescription,parentRoleID,CardTypeManagerId) VALUES(2,'超级管理员','人员管理',null,1)
INSERT dbo.Role(id,RoleName, RoleDescription,parentRoleID,CardTypeManagerId) VALUES(3,'超级管理员','设备管理',null,2)
INSERT dbo.Role(id,RoleName, RoleDescription,parentRoleID,CardTypeManagerId) VALUES(4,'管理员','人员和设备管理',1,3)
INSERT dbo.Role(id,RoleName, RoleDescription,parentRoleID,CardTypeManagerId) VALUES(5,'管理员','人员管理',2,1)
INSERT dbo.Role(id,RoleName, RoleDescription,parentRoleID,CardTypeManagerId) VALUES(6,'管理员','设备管理',3,2)
--INSERT dbo.Role(RoleID,RoleName, RoleDescription,parentRoleID) VALUES(3,'人员管理员','人员管理',2)
--INSERT dbo.Role(RoleID,RoleName, RoleDescription,parentRoleID) VALUES(4,'物品管理员','设备管理',2)
INSERT dbo.Role(id,RoleName, RoleDescription,parentRoleID,CardTypeManagerId) VALUES(7,'一般用户','具有人员和设备查看权限',4,3)
INSERT dbo.Role(id,RoleName, RoleDescription,parentRoleID,CardTypeManagerId) VALUES(8,'一般用户','只具有人员查看权限',5,1)
INSERT dbo.Role(id,RoleName, RoleDescription,parentRoleID,CardTypeManagerId) VALUES(9,'一般用户','只具有查看权限',6,2)
GO

--userRoleRelation
INSERT into userRoleRelation(userId,roleId)  VALUES(1,1);
go

--actionType
INSERT dbo.actionType(id,code, name) VALUES(1,'ADD','增加权限')
INSERT dbo.actionType(id,code, name) VALUES(2,'DEL','删除权限')
INSERT dbo.actionType(id,code, name) VALUES(3,'EDIT','编辑权限')
INSERT dbo.actionType(id,code, name) VALUES(4,'QUERY','查询权限')
Go

--Module
insert into Module(id,code,name)	VALUES(1,'USER','用户');
insert into Module(id,code,name)	VALUES(2,'BASESTATION','基站');
insert into Module(id,code,name)	VALUES(3,'PERSON','人员卡');
insert into Module(id,code,name)	VALUES(4,'GOODS','物品卡');
insert into Module(id,code,name)	VALUES(5,'AREA','区域卡');
insert into Module(id,code,name)	VALUES(6,'AREATYPE','区域类型');
insert into Module(id,code,name)	VALUES(7,'DEPART','科室部门');
insert into Module(id,code,name)	VALUES(8,'BSLINKALARM','基站链路报警');
insert into Module(id,code,name)	VALUES(9,'PERSONALARM','人员报警');
insert into Module(id,code,name)	VALUES(10,'DEVICESOSALARM','设备报警');
insert into Module(id,code,name)	VALUES(11,'ROLECONFIG','权限管理');
Go

--action
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(1,'增加用户',1,1)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(2,'删除用户',2,1)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(3,'编辑用户',3,1)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(4,'查询用户',4,1)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(5,'增加基站',1,2)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(6,'删除基站',2,2)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(7,'编辑基站',3,2)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(8,'查询基站',4,2)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(9,'增加人员',1,3)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(10,'删除人员',2,3)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(11,'编辑人员',3,3)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(12,'查询人员',4,3)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(13,'增加物品',1,4)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(14,'删除物品',2,4)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(15,'编辑物品',3,4)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(16,'查询物品',4,4)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(17,'增加区域',1,5)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(18,'删除区域',2,5)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(19,'编辑区域',3,5)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(20,'查询区域',4,5)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(21,'增加区域类型',1,6)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(22,'删除区域类型',2,6)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(23,'编辑区域类型',3,6)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(24,'查询区域类型',4,6)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(25,'增加科室',1,7)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(26,'删除科室',2,7)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(27,'编辑科室',3,7)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(28,'查询科室',4,7)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(29,'增加基站报警',1,8)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(30,'删除基站报警',2,8)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(31,'编辑基站报警',3,8)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(32,'查询基站报警',4,8)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(33,'增加人员报警',1,9)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(34,'删除人员报警',2,9)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(35,'编辑人员报警',3,9)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(36,'查询人员报警',4,9)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(37,'增加设备报警',1,10)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(38,'删除设备报警',2,10)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(39,'编辑设备报警',3,10)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(40,'查询设备报警',4,10)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(41,'用户权限分配',1,11)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(42,'删除用户权限',2,11)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(43,'编辑用户权限',3,11)
INSERT dbo.action(id,ActionName, actionTypeId,moduleId) VALUES(44,'查询用户权限',4,11)
Go

--RoleActionRelation
--超级管理员
insert into	RoleActionRelation(RoleId,ActionId)	values(1,1);
insert into	RoleActionRelation(RoleId,ActionId)	values(1,2);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,3);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,4);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,5);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,6);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,7);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,8);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,9);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,10);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,11);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,12);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,13);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,14);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,15);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,16);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,17);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,18);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,19);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,20);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,21);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,22);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,23);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,24);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,25);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,26);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,27);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,28);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,29);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,30);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,31);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,32);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,33);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,34);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,35);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,36);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,37);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,38);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,39);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,40);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,41);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,42);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,43);
insert into	RoleActionRelation(RoleId,ActionId)	values (1,44);
--人员超级管理员
insert into	RoleActionRelation(RoleId,ActionId)	values(2,1);
insert into	RoleActionRelation(RoleId,ActionId)	values(2,2);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,3);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,4);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,5);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,6);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,7);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,8);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,9);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,10);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,11);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,12);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,17);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,18);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,19);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,20);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,21);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,22);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,23);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,24);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,25);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,26);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,27);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,28);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,29);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,30);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,31);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,32);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,33);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,34);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,35);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,36);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,41);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,42);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,43);
insert into	RoleActionRelation(RoleId,ActionId)	values (2,44);
--物品超级管理员
insert into	RoleActionRelation(RoleId,ActionId)	values(3,1);
insert into	RoleActionRelation(RoleId,ActionId)	values(3,2);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,3);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,4);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,5);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,6);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,7);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,8);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,13);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,14);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,15);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,16);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,17);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,18);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,19);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,20);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,21);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,22);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,23);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,24);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,25);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,26);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,27);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,28);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,29);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,30);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,31);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,32);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,37);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,38);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,39);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,40);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,41);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,42);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,43);
insert into	RoleActionRelation(RoleId,ActionId)	values (3,44);
--管理员
insert into	RoleActionRelation(RoleId,ActionId)	values(4,1);
insert into	RoleActionRelation(RoleId,ActionId)	values(4,2);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,3);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,4);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,5);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,6);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,7);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,8);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,9);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,10);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,11);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,12);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,13);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,14);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,15);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,16);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,17);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,18);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,19);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,20);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,21);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,22);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,23);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,24);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,25);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,26);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,27);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,28);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,29);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,30);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,31);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,32);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,33);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,34);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,35);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,36);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,37);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,38);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,39);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,40);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,41);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,42);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,43);
insert into	RoleActionRelation(RoleId,ActionId)	values (4,44);

--人员管理员
insert into	RoleActionRelation(RoleId,ActionId)	values(5,1);
insert into	RoleActionRelation(RoleId,ActionId)	values(5,2);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,3);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,4);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,5);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,6);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,7);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,8);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,9);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,10);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,11);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,12);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,17);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,18);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,19);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,20);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,21);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,22);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,23);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,24);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,25);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,26);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,27);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,28);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,29);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,30);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,31);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,32);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,33);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,34);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,35);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,36);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,41);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,42);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,43);
insert into	RoleActionRelation(RoleId,ActionId)	values (5,44);
--物品管理员
insert into	RoleActionRelation(RoleId,ActionId)	values(6,1);
insert into	RoleActionRelation(RoleId,ActionId)	values(6,2);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,3);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,4);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,5);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,6);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,7);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,8);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,13);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,14);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,15);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,16);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,17);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,18);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,19);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,20);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,21);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,22);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,23);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,24);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,25);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,26);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,27);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,28);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,29);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,30);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,31);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,32);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,37);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,38);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,39);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,40);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,41);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,42);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,43);
insert into	RoleActionRelation(RoleId,ActionId)	values (6,44);
--一般用户
insert into	RoleActionRelation(RoleId,ActionId)	values (7,12);
insert into	RoleActionRelation(RoleId,ActionId)	values (7,16);
insert into	RoleActionRelation(RoleId,ActionId)	values (7,36);
insert into	RoleActionRelation(RoleId,ActionId)	values (7,40);

insert into	RoleActionRelation(RoleId,ActionId)	values (8,12);
insert into	RoleActionRelation(RoleId,ActionId)	values (8,36);
--insert into	RoleActionRelation(RoleId,ActionId)	values (8,16);
--insert into	RoleActionRelation(RoleId,ActionId)	values (9,12);
insert into	RoleActionRelation(RoleId,ActionId)	values (9,16);
insert into	RoleActionRelation(RoleId,ActionId)	values (9,40);
Go

--Settings
execute sp_addextendedproperty 'MS_Description', 
   '系统参数表',
   'user', 'dbo', 'table', 'Settings'
go

execute sp_addextendedproperty 'MS_Description', 
   '键',
   'user', 'dbo', 'table', 'Settings', 'column', 'Item'
go

execute sp_addextendedproperty 'MS_Description', 
   '值',
   'user', 'dbo', 'table', 'Settings', 'column', 'Value'
go

execute sp_addextendedproperty 'MS_Description', 
   '注释说明',
   'user', 'dbo', 'table', 'Settings', 'column', 'Description'
go
--Settings
INSERT dbo.Settings(Item, Value, Description) 
select 'FileGroupPath', 'E:\MSSQL_Data\db\LPBSS\', '文件分组目录' union
select 'version', 'main', '数据库版本号'union
select 'licence', '', 'licence序列号'union
select 'stime', '9wYayzKLDY9l2AZ638s0WBeTrzCIKURK', 'licence序列号时间'union
select 'WorkSitePass', 'STime', 'table' union
select 'WorksiteAbnormalTimeSpan', '10', '超过10分钟,基站判定为异常' union
select 'NoSignalShowTimeSpan', '300', '人员模型无信号后要消失在地图上的等待时间(分钟)' union
select 'IsNoSignalAlarm', '1', '1则启动无信号报警' union
select 'IsStartupPurview', '1', '1则启动权限' union
select 'title', '翌日科技智慧养老综合管理系统', '系统标题' union
select 'IsAreaAccessAlarm', '1', '1则启动区域准入报警' union
select 'IsAreaOverTimeAlarm', '1', '1则启动区域超时报警' union
select 'IsTumbleAlarm', '1', '1则启动跌倒报警' union
select 'IsCardSilentAlarm', '1', '1则启动异常静止报警' union
select 'IsCardDisConAlarm', '1', '1则启动拆卸报警' union
select 'IsAutoDisplayFaultAlarm', '1', '1则启动自动消警--故障报警' union
select 'IsAutoDisplayNoSignalAlarm', '1', '1则启动自动消警--无信号' union
select 'IsCardLowPowerAlarm', '1', '1则启动卡低电报警' union
select 'IsFaultAlarm', '1', '1则启动故障报警' union
select 'IsCallAlarm', '1', '1则启动呼叫报警' union
--select 'IsCallLowPowerAlarm', '1', '1则启动呼叫器低电报警' union
select 'IsOutBedAlarm', '1', '1则启动离床报警' union
select 'IsNoSignalAlarm', '1', '1则启动无信号报警' union
select 'IsSOSAlarm', '1', '1则启动人员求救报警' union
select 'Subtitle', '深圳市翌日科技有限公司承制', '副标题' union
select 'CardSilentSpan', '08:00-22:00', '卡异常报警时间段,逗号隔开' union
select 'CardDisConSpan', '08:00-22:00', '防拆卸报警时间段,逗号隔开' union
select 'OutBedAlarmSpan', '08:00-22:00', '离床报警时间段,逗号隔开' union
select 'HisTrackType', '0', '历史轨迹类型：0表示基站经过，1表示历史轨迹点' union
select 'IsCallAlarm', '1' , '1则启动呼叫报警' union
select 'IsOffBedAlarm', '1', '1则启动离床报警'
GO

--select * from users
insert into users(ID, Code, Name, Password,CanLogin,Disabled, description, Modifier, ModifyTime)
select '0B9BA5A7-C558-442A-8E74-4D3B5388B7B2','Admin','Admin','58-A8-A1-F9-A0-69-ED-16',1,0,'备注',null,'2013-05-02 00:00:00.000'
GO
--select * from roles
insert into Roles(ID,Name,ParentID,Disabled)
select '3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','管理员','00000000-0000-0000-0000-000000000000',0 
GO
--select * from UserRole
insert into UserRole
select '0B9BA5A7-C558-442A-8E74-4D3B5388B7B2','3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084'
GO
--select * from operate
insert into operate(ID,Name)
select '41283AEB-EB9F-45DF-9A40-83F10B95063D','查询' union
select '187BB482-F59D-4EE1-939C-3C64AC1BB36B','新增' union
select '0D755E95-BF99-495F-9506-27BC7D8EC904','修改' union
select '750BE12D-8589-4D18-A20F-EC7B1931A167','删除' union
select '336D8E5A-FE71-48CE-A8DF-5CDB08A40C6C','保存' union
select '2D43F62D-5799-4D1E-A9D0-63BF4EDC2633','导入' union
select '4BFD4B20-5B4A-4DDD-9D08-013FF8C56875','导出' union
select '3799C387-8EC3-450A-9DB5-B2C724B0C0B2','打印' union
select 'D03437ED-C860-4A17-A40F-555201B4273E','阅读' union
select '93A596E2-1C59-49F8-941D-E5A06C60F5FE','定位' union
select '50110051-0239-4D6A-BFFE-0BAE3B74306E','发送' union
select 'F3D8093F-3740-4575-9102-F26AA36BE804','轨迹播放' union
select '6790B67D-E202-4C00-BA8E-8F8FD8091068','下发撤离' union
select '5B64FB30-1B0D-4D1F-996A-6A403193FB86','取消撤离'
GO


--插入菜单信息
--delete from MenuList
--系统设置--68BCD668-8BEB-4AEA-BDB9-E50E70407792 
--报警设置--C235000A-AC35-41AF-876E-2E6D3B2172B1
set identity_insert MenuList ON
insert into MenuList(ID, Name, ParentID, SerialNum, IsBizModule, NameSpace, ViewName,IconResourceName,HIconResourceName,OperateListID,Disabled, Description,PageID)
--信息查询0
select '5BAE515B-651E-4A0C-A013-204D84D58FA3','信息查询','00000000-0000-0000-0000-000000000000',0,0,null,null,'serchInfo.png','HserchInfo.png',null,0,'信息查询',1 union
--select '25A1EC10-D934-4AC7-8156-AE6F53C314FB','设备信息查询','5BAE515B-651E-4A0C-A013-204D84D58FA3',1,1,'LPBSS.Module.BaseData','DeviceBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,2d43f62d-5799-4d1e-a9d0-63bf4edc2633,41283aeb-eb9f-45df-9a40-83f10b95063d,3799c387-8ec3-450a-9db5-b2c724b0c0b2,750be12d-8589-4d18-a20f-ec7b1931a167,93A596E2-1C59-49F8-941D-E5A06C60F5FE,F3D8093F-3740-4575-9102-F26AA36BE804',0,'设备信息查询',2 union
select 'F4818CA9-039B-4ECA-9440-551D78693743','老人信息查询','5BAE515B-651E-4A0C-A013-204D84D58FA3',0,1,'LPBSS.Module.BaseData','OldPersonBrowse',null,null,'41283AEB-EB9F-45DF-9A40-83F10B95063D',0,'老人信息查询',3 union
select '9353A417-BC80-4526-A10A-2C0F600D9EA6','员工信息查询','5BAE515B-651E-4A0C-A013-204D84D58FA3',1,1,'LPBSS.Module.BaseData','PersonOnlyBrowse',null,null,'41283aeb-eb9f-45df-9a40-83f10b95063d',0,'员工信息查询',4 union
select '2F22F596-9DC4-47DD-A2AE-5FCBC42BD50B','人员轨迹查询','5BAE515B-651E-4A0C-A013-204D84D58FA3',3,1,'LPBSS.Module.SearchInfo','HistoryPathBrowseView',null,null,'41283aeb-eb9f-45df-9a40-83f10b95063d,F3D8093F-3740-4575-9102-F26AA36BE804',0,'人员轨迹查询',5 union
select '573BA43E-0B55-40F5-AAE5-C77D77CBA02A','日志信息查询','5BAE515B-651E-4A0C-A013-204D84D58FA3',4,1,'LPBSS.Module.Settings','OperationLogBrowseView',null,null,'41283aeb-eb9f-45df-9a40-83f10b95063d',0,'日志信息查询',6 union
select '4555322F-700A-4DAE-A336-9EE2AAD49F9D','撤离信息查询','5BAE515B-651E-4A0C-A013-204D84D58FA3',5,1,'LPBSS.Module.SearchInfo','EvacuateInfoView',null,null,'41283AEB-EB9F-45DF-9A40-83F10B95063D,6790B67D-E202-4C00-BA8E-8F8FD8091068,5B64FB30-1B0D-4D1F-996A-6A403193FB86',1,'撤离信息查询',7 union
select 'B5F12B3E-0ECC-4F39-95F7-C00EFD08B116','紧急撤离','00000000-0000-0000-0000-000000000000',6,1,'LPBSS.Module.SearchInfo','AddEvacuateInfo',null,null,'41283AEB-EB9F-45DF-9A40-83F10B95063D',1,'紧急撤离',8 union
select '447BA84E-7634-4001-AF67-E5AEFD67E44F','系统短信查询','5BAE515B-651E-4A0C-A013-204D84D58FA3',7,1,'LPBSS.Module.SearchInfo','DownMsgBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,41283aeb-eb9f-45df-9a40-83f10b95063d,50110051-0239-4D6A-BFFE-0BAE3B74306E',0,'系统短信查询',9 union
select 'E4BB96A9-8D56-43E5-A8F0-4BE00B804CFC','运动状态查询','5BAE515B-651E-4A0C-A013-204D84D58FA3',8,1,'LPBSS.Module.SearchInfo','CardExerciseChartView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167',1,'运动状态查询',10 union
select 'F39AAF06-3FD0-4B09-88C4-380A7A96A46E','联动报警查询','5BAE515B-651E-4A0C-A013-204D84D58FA3',5,1,'LPBSS.Module.SearchInfo','ChestCardDownMsgBrowse',null,null,'41283aeb-eb9f-45df-9a40-83f10b95063d,4bfd4b20-5b4a-4ddd-9d08-013ff8c56875',0,'联动报警查询',11 union
--报警管理1 
select 'C235000A-AC35-41AF-876E-2E6D3B2172B1','报警管理','00000000-0000-0000-0000-000000000000',1,0,null,null,'alarm.png','Hararm.png',null,0,'报警管理',20 union
select '87041CAB-942B-4BBA-B77A-4DF57946F45C','故障报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',9,1,'LPBSS.Module.AlarmManager','FaultAlarmBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,41283aeb-eb9f-45df-9a40-83f10b95063d, 93A596E2-1C59-49F8-941D-E5A06C60F5FE',0,'故障报警',22 union
select '9F97EFA4-78FD-4333-B5E7-8C98B8FDF208','无信号报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',6,1,'LPBSS.Module.AlarmManager','NoSignAlarmBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,,41283aeb-eb9f-45df-9a40-83f10b95063d',0,'无信号报警',23 union
select '0D72FFFE-3274-4294-8CF8-8E08DE228316','卡低电报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',7,1,'LPBSS.Module.AlarmManager','CardLowPowerAlarmBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,41283aeb-eb9f-45df-9a40-83f10b95063d',0,'卡低电报警',24 union
select 'B6A66FAE-7168-43D8-9687-B1D3E5502BFB','求救报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',0,1,'LPBSS.Module.AlarmManager','SOSAlarmBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,,41283aeb-eb9f-45df-9a40-83f10b95063d, 93A596E2-1C59-49F8-941D-E5A06C60F5FE',0,'求救报警',25 union
select '2F6D32A3-B91F-4A95-ADCD-50C79844435F','区域准入报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',10,1,'LPBSS.Module.AlarmManager','AreaAccessBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,41283aeb-eb9f-45df-9a40-83f10b95063d,93A596E2-1C59-49F8-941D-E5A06C60F5FE',0,'区域准入报警',26 union
select 'C73A1A53-F0A4-41BB-A712-7A2DD5328715','区域超时报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',11,1,'LPBSS.Module.AlarmManager','AreaOvertimeBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,41283aeb-eb9f-45df-9a40-83f10b95063d,93A596E2-1C59-49F8-941D-E5A06C60F5FE,',0,'区域超时报警',27 union
select '7AD12DDC-F3B2-4E72-9025-6292B06304C3','跌倒报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',3,1,'LPBSS.Module.AlarmManager','TumbleAlarmBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,41283aeb-eb9f-45df-9a40-83f10b95063d,93A596E2-1C59-49F8-941D-E5A06C60F5FE,',0,'跌倒报警',28 union
select '1C89810E-59C1-4C28-83FF-7CF13FE99636','异常静止报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',5,1,'LPBSS.Module.AlarmManager','AbnormalMotionlessAlarmBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,41283aeb-eb9f-45df-9a40-83f10b95063d,93A596E2-1C59-49F8-941D-E5A06C60F5FE,',0,'异常静止报警',29 union
select '532CB28D-A104-4E28-B661-33D07218BD43','拆卸报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',4,1,'LPBSS.Module.AlarmManager','DisassemblyAlarmBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,41283aeb-eb9f-45df-9a40-83f10b95063d,93A596E2-1C59-49F8-941D-E5A06C60F5FE,',0,'拆卸报警',30 union
select '93F6EB1C-F64B-4AF3-84F0-97CA6E0B4BEA','呼叫报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',1,1,'LPBSS.Module.AlarmManager','CallAlarmBrowse',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,41283aeb-eb9f-45df-9a40-83f10b95063d',0,'呼叫报警',31 union
select '22745622-6940-440D-A886-8ADAC5851B8E','离床报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',2,1,'LPBSS.Module.AlarmManager','LeaveBedAlarmBrowse',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,41283aeb-eb9f-45df-9a40-83f10b95063d',0,'离床报警',32 union
--select 'F25B43B7-F949-4BDC-9337-A790DB16A527','呼叫器低电报警','C235000A-AC35-41AF-876E-2E6D3B2172B1',8,1,'LPBSS.Module.AlarmManager','PagerLowerAlarmView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,d03437ed-c860-4a17-a40f-555201b4273e,41283aeb-eb9f-45df-9a40-83f10b95063d',0,'呼叫器低电报警',33 union
--信息维护2
select '6516D828-5B78-44BC-8C02-BCDB2C5DE28D','信息维护','00000000-0000-0000-0000-000000000000',2,0,null,null,'basicData.png','HbaseData.png',null,0,'信息维护',40 union
select '7D1FFE10-427D-4CB5-ABB5-9AABB529DFB5','员工信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',1,1,'LPBSS.Module.BaseData','PersonBrowse',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,,,2d43f62d-5799-4d1e-a9d0-63bf4edc2633,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167,F3D8093F-3740-4575-9102-F26AA36BE804',0,'员工信息',41 union
select 'C1F7782E-7479-41FB-BE79-3466CAEE53D1','职务信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',3,1,'LPBSS.Module.BaseData','OfficePositionBrowse',null,null,'0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167',0,'职务信息',42 union
select '30D7F8B6-C60C-42C0-ABA6-DC9C167D2207','部门信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',4,1,'LPBSS.Module.BaseData','DepartmentBrowse',null,null,'0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,41283aeb-eb9f-45df-9a40-83f10b95063d,,750be12d-8589-4d18-a20f-ec7b1931a167',0,'科室信息',43 union
--select 'CFA018E2-1F84-4AB8-82D9-09120C332523','设备信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',3,1,'LPBSS.Module.BaseData','DeviceBrowseView',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,,,2d43f62d-5799-4d1e-a9d0-63bf4edc2633,41283aeb-eb9f-45df-9a40-83f10b95063d,3799c387-8ec3-450a-9db5-b2c724b0c0b2,750be12d-8589-4d18-a20f-ec7b1931a167,93A596E2-1C59-49F8-941D-E5A06C60F5FE,F3D8093F-3740-4575-9102-F26AA36BE804',0,'设备信息',44 union
--select 'E8C7B9D3-14E3-4E0E-BBBA-C5BEE7503555','设备类型信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',4,1,'LPBSS.Module.BaseData','DeviceTypeBrowseView',null,null,'0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,41283aeb-eb9f-45df-9a40-83f10b95063d,,750be12d-8589-4d18-a20f-ec7b1931a167',0,'设备类型信息',45 union
select '6E75A9C1-DC26-4E09-92F2-42D1654C8419','区域信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',5,1,'LPBSS.Module.BaseData','AreaBrowse',null,null,'0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167',0,'区域信息',46 union
select 'AC638CE6-FFA0-4B7E-B866-A36E6D28B53A','区域类型信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',6,1,'LPBSS.Module.BaseData','AreaTypeBrowse',null,null,'0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167',0,'区域类型信息',47 union
select '56CA0AAD-9A0E-4B58-8E3D-350FE6993A1D','定位基站信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',7,1,'LPBSS.Module.BaseData','WorkSiteBrowse',null,null,'0d755e95-bf99-495f-9506-27bc7d8ec904,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167,93A596E2-1C59-49F8-941D-E5A06C60F5FE',0,'定位基站信息',48 union
select '77442D96-A9C9-4A17-A170-D7E14216F4A5','基站控制单元','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',8,1,'LPBSS.Module.BaseData','WorkSiteControlBrowse',null,null,'0d755e95-bf99-495f-9506-27bc7d8ec904,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167',0,'基站控制单元',49 union
select '2C801475-C3D4-47CC-9B0A-0769B85897B8','摄像头信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',9,1,'LPBSS.Module.BaseData','CameraControlBrowse',null,null,'0d755e95-bf99-495f-9506-27bc7d8ec904,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167,93A596E2-1C59-49F8-941D-E5A06C60F5FE',0,'摄像头信息',50 union
select '2FAB1D28-FB72-4F86-8438-C1715FF99150','老人信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',0,1,'LPBSS.Module.BaseData','OldPersonInfoBrowse',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,,,2d43f62d-5799-4d1e-a9d0-63bf4edc2633,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167,F3D8093F-3740-4575-9102-F26AA36BE804',0,'老人信息',51 union
select '1892E0A1-321C-42FB-98EE-5C3DFE509113','关联信息','6516D828-5B78-44BC-8C02-BCDB2C5DE28D',2,1,'LPBSS.Module.BaseData','AllCareInfoBrowse',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,,,2d43f62d-5799-4d1e-a9d0-63bf4edc2633,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167,F3D8093F-3740-4575-9102-F26AA36BE804',0,'综合护理信息',52 union
--系统设置3
select '68BCD668-8BEB-4AEA-BDB9-E50E70407792','系统设置','00000000-0000-0000-0000-000000000000',3,0,null,null,'settings.png','Hsettings.png',null,0,'系统设置',60 union
select '669C0624-0315-476F-AE23-9A23BF08BA52','用户信息','68BCD668-8BEB-4AEA-BDB9-E50E70407792',0,1,'LPBSS.Module.Permission','UsersBrowse',null,null,'4bfd4b20-5b4a-4ddd-9d08-013ff8c56875,0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,,,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167',0,'用户信息模块',61 union
select '9172722B-43E3-473A-B6F9-2BC73792323D','角色信息','68BCD668-8BEB-4AEA-BDB9-E50E70407792',1,1,'LPBSS.Module.Permission','RoleBrowse',null,null,'0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167',0,'角色信息',62 union
select '9A22C361-A606-430D-A72D-368DC47E8567','角色权限','68BCD668-8BEB-4AEA-BDB9-E50E70407792',2,1,'LPBSS.Module.Permission','RolePermission',null,null,'336d8e5a-fe71-48ce-a8df-5cdb08a40c6c,41283aeb-eb9f-45df-9a40-83f10b95063d',1,'角色权限',63 union
select 'D25042F3-C5EF-41A4-B8D9-1BDB0219ECFF','模块信息','68BCD668-8BEB-4AEA-BDB9-E50E70407792',3,1,'LPBSS.Module.Permission','ModuleName',null,null,'0d755e95-bf99-495f-9506-27bc7d8ec904,187bb482-f59d-4ee1-939c-3c64ac1bb36b,41283aeb-eb9f-45df-9a40-83f10b95063d',1,'模块信息',64 union
select 'E3B1E2E8-D61B-456E-A644-25F8802DED9A','快捷菜单','68BCD668-8BEB-4AEA-BDB9-E50E70407792',4,1,'LPBSS.Module.Permission','MenuFavoriteBrowse',null,null,'41283aeb-eb9f-45df-9a40-83f10b95063d,750be12d-8589-4d18-a20f-ec7b1931a167,187bb482-f59d-4ee1-939c-3c64ac1bb36b',0,'快捷菜单',65 union
select '799063F1-ACBE-4F2F-A2D2-A7D36A254AEA','操作名称','68BCD668-8BEB-4AEA-BDB9-E50E70407792',5,1,'LPBSS.Module.Permission','OperateBrowse',null,null,'41283aeb-eb9f-45df-9a40-83f10b95063d',1,'操作信息',66 union
select 'EEA6CCF5-80FC-47D7-8FF5-D5A0AE816C43','参数设置','68BCD668-8BEB-4AEA-BDB9-E50E70407792',6,1,'LPBSS.Module.Settings','ManageSettings',null,null,'41283aeb-eb9f-45df-9a40-83f10b95063d,0d755e95-bf99-495f-9506-27bc7d8ec904',0,'参数设置',67 union
select '02A636F7-FD97-4DF7-80CB-25A768233CCE','用户权限','68BCD668-8BEB-4AEA-BDB9-E50E70407792',7,1,'LPBSS.Module.Permission','UserRole',null,null,null,1,'用户权限',68

GO

--select * from operate
--delete from RolePermission where MenuListID = '799063F1-ACBE-4F2F-A2D2-A7D36A254AEA'
insert into RolePermission(ID,RoleID,MenuListID,OperateID)
--摄像头信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2C801475-C3D4-47CC-9B0A-0769B85897B8','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2C801475-C3D4-47CC-9B0A-0769B85897B8','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2C801475-C3D4-47CC-9B0A-0769B85897B8','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2C801475-C3D4-47CC-9B0A-0769B85897B8','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2C801475-C3D4-47CC-9B0A-0769B85897B8','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union

--信息查询-工作人员信息
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9353A417-BC80-4526-A10A-2C0F600D9EA6','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9353A417-BC80-4526-A10A-2C0F600D9EA6','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9353A417-BC80-4526-A10A-2C0F600D9EA6','750BE12D-8589-4D18-A20F-EC7B1931A167' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9353A417-BC80-4526-A10A-2C0F600D9EA6','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9353A417-BC80-4526-A10A-2C0F600D9EA6','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9353A417-BC80-4526-A10A-2C0F600D9EA6','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9353A417-BC80-4526-A10A-2C0F600D9EA6','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9353A417-BC80-4526-A10A-2C0F600D9EA6','F3D8093F-3740-4575-9102-F26AA36BE804' union

--信息查询-老人信息查询
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','F4818CA9-039B-4ECA-9440-551D78693743','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','F4818CA9-039B-4ECA-9440-551D78693743','41283AEB-EB9F-45DF-9A40-83F10B95063D' union


--信息查询-人员轨迹轨迹
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2F22F596-9DC4-47DD-A2AE-5FCBC42BD50B','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2F22F596-9DC4-47DD-A2AE-5FCBC42BD50B','F3D8093F-3740-4575-9102-F26AA36BE804' union
--信息查询-日志信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','573BA43E-0B55-40F5-AAE5-C77D77CBA02A','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','573BA43E-0B55-40F5-AAE5-C77D77CBA02A','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','573BA43E-0B55-40F5-AAE5-C77D77CBA02A','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union

--信息查询-胸牌下发信息查询
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','F39AAF06-3FD0-4B09-88C4-380A7A96A46E','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','F39AAF06-3FD0-4B09-88C4-380A7A96A46E','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','F39AAF06-3FD0-4B09-88C4-380A7A96A46E','50110051-0239-4D6A-BFFE-0BAE3B74306E' union


--信息查询-下发信息查询
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','447BA84E-7634-4001-AF67-E5AEFD67E44F','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','447BA84E-7634-4001-AF67-E5AEFD67E44F','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','447BA84E-7634-4001-AF67-E5AEFD67E44F','50110051-0239-4D6A-BFFE-0BAE3B74306E' union

--信息查询-运动状态查询
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','E4BB96A9-8D56-43E5-A8F0-4BE00B804CFC','41283AEB-EB9F-45DF-9A40-83F10B95063D' union

--信息查询-设备盘点
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','A6FDE835-2FE0-4BAC-95A1-39A65DB6C0B9','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--信息查询-撤离信息查询
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','4555322F-700A-4DAE-A336-9EE2AAD49F9D','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','4555322F-700A-4DAE-A336-9EE2AAD49F9D','5B64FB30-1B0D-4D1F-996A-6A403193FB86' union
--紧急撤离
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','B5F12B3E-0ECC-4F39-95F7-C00EFD08B116','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--报警管理
--故障报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','87041CAB-942B-4BBA-B77A-4DF57946F45C','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','87041CAB-942B-4BBA-B77A-4DF57946F45C','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','87041CAB-942B-4BBA-B77A-4DF57946F45C','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','87041CAB-942B-4BBA-B77A-4DF57946F45C','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','87041CAB-942B-4BBA-B77A-4DF57946F45C','3799C387-8EC3-450A-9DB5-B2C724B0C0B2' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','87041CAB-942B-4BBA-B77A-4DF57946F45C','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--卡低电报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','0D72FFFE-3274-4294-8CF8-8E08DE228316','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','0D72FFFE-3274-4294-8CF8-8E08DE228316','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','0D72FFFE-3274-4294-8CF8-8E08DE228316','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','0D72FFFE-3274-4294-8CF8-8E08DE228316','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','0D72FFFE-3274-4294-8CF8-8E08DE228316','3799C387-8EC3-450A-9DB5-B2C724B0C0B2' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','0D72FFFE-3274-4294-8CF8-8E08DE228316','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--区域准入报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2F6D32A3-B91F-4A95-ADCD-50C79844435F','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2F6D32A3-B91F-4A95-ADCD-50C79844435F','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2F6D32A3-B91F-4A95-ADCD-50C79844435F','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2F6D32A3-B91F-4A95-ADCD-50C79844435F','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2F6D32A3-B91F-4A95-ADCD-50C79844435F','3799C387-8EC3-450A-9DB5-B2C724B0C0B2' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2F6D32A3-B91F-4A95-ADCD-50C79844435F','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--区域超时报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','C73A1A53-F0A4-41BB-A712-7A2DD5328715','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','C73A1A53-F0A4-41BB-A712-7A2DD5328715','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','C73A1A53-F0A4-41BB-A712-7A2DD5328715','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','C73A1A53-F0A4-41BB-A712-7A2DD5328715','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','C73A1A53-F0A4-41BB-A712-7A2DD5328715','3799C387-8EC3-450A-9DB5-B2C724B0C0B2' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','C73A1A53-F0A4-41BB-A712-7A2DD5328715','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--无信号报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9F97EFA4-78FD-4333-B5E7-8C98B8FDF208','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9F97EFA4-78FD-4333-B5E7-8C98B8FDF208','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9F97EFA4-78FD-4333-B5E7-8C98B8FDF208','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9F97EFA4-78FD-4333-B5E7-8C98B8FDF208','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9F97EFA4-78FD-4333-B5E7-8C98B8FDF208','3799C387-8EC3-450A-9DB5-B2C724B0C0B2' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9F97EFA4-78FD-4333-B5E7-8C98B8FDF208','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--求救报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','B6A66FAE-7168-43D8-9687-B1D3E5502BFB','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','B6A66FAE-7168-43D8-9687-B1D3E5502BFB','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','B6A66FAE-7168-43D8-9687-B1D3E5502BFB','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','B6A66FAE-7168-43D8-9687-B1D3E5502BFB','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','B6A66FAE-7168-43D8-9687-B1D3E5502BFB','3799C387-8EC3-450A-9DB5-B2C724B0C0B2' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','B6A66FAE-7168-43D8-9687-B1D3E5502BFB','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--跌倒报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7AD12DDC-F3B2-4E72-9025-6292B06304C3','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7AD12DDC-F3B2-4E72-9025-6292B06304C3','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7AD12DDC-F3B2-4E72-9025-6292B06304C3','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7AD12DDC-F3B2-4E72-9025-6292B06304C3','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--异常静止报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1C89810E-59C1-4C28-83FF-7CF13FE99636','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1C89810E-59C1-4C28-83FF-7CF13FE99636','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1C89810E-59C1-4C28-83FF-7CF13FE99636','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1C89810E-59C1-4C28-83FF-7CF13FE99636','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--拆卸报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','532CB28D-A104-4E28-B661-33D07218BD43','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','532CB28D-A104-4E28-B661-33D07218BD43','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','532CB28D-A104-4E28-B661-33D07218BD43','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','532CB28D-A104-4E28-B661-33D07218BD43','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union

--呼叫报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','93F6EB1C-F64B-4AF3-84F0-97CA6E0B4BEA','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','93F6EB1C-F64B-4AF3-84F0-97CA6E0B4BEA','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','93F6EB1C-F64B-4AF3-84F0-97CA6E0B4BEA','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','93F6EB1C-F64B-4AF3-84F0-97CA6E0B4BEA','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union

--离床报警
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','22745622-6940-440D-A886-8ADAC5851B8E','D03437ED-C860-4A17-A40F-555201B4273E' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','22745622-6940-440D-A886-8ADAC5851B8E','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','22745622-6940-440D-A886-8ADAC5851B8E','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','22745622-6940-440D-A886-8ADAC5851B8E','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union

--呼叫器低电报警
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','F25B43B7-F949-4BDC-9337-A790DB16A527','D03437ED-C860-4A17-A40F-555201B4273E' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','F25B43B7-F949-4BDC-9337-A790DB16A527','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','F25B43B7-F949-4BDC-9337-A790DB16A527','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','F25B43B7-F949-4BDC-9337-A790DB16A527','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union


--信息维护-工作人员信息
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7D1FFE10-427D-4CB5-ABB5-9AABB529DFB5','3799C387-8EC3-450A-9DB5-B2C724B0C0B2' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7D1FFE10-427D-4CB5-ABB5-9AABB529DFB5','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7D1FFE10-427D-4CB5-ABB5-9AABB529DFB5','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7D1FFE10-427D-4CB5-ABB5-9AABB529DFB5','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7D1FFE10-427D-4CB5-ABB5-9AABB529DFB5','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7D1FFE10-427D-4CB5-ABB5-9AABB529DFB5','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7D1FFE10-427D-4CB5-ABB5-9AABB529DFB5','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7D1FFE10-427D-4CB5-ABB5-9AABB529DFB5','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','7D1FFE10-427D-4CB5-ABB5-9AABB529DFB5','F3D8093F-3740-4575-9102-F26AA36BE804' union

--信息维护-老人信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2FAB1D28-FB72-4F86-8438-C1715FF99150','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2FAB1D28-FB72-4F86-8438-C1715FF99150','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2FAB1D28-FB72-4F86-8438-C1715FF99150','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2FAB1D28-FB72-4F86-8438-C1715FF99150','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2FAB1D28-FB72-4F86-8438-C1715FF99150','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2FAB1D28-FB72-4F86-8438-C1715FF99150','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','2FAB1D28-FB72-4F86-8438-C1715FF99150','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union

--信息维护-综合护理信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1892E0A1-321C-42FB-98EE-5C3DFE509113','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1892E0A1-321C-42FB-98EE-5C3DFE509113','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1892E0A1-321C-42FB-98EE-5C3DFE509113','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1892E0A1-321C-42FB-98EE-5C3DFE509113','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1892E0A1-321C-42FB-98EE-5C3DFE509113','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1892E0A1-321C-42FB-98EE-5C3DFE509113','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','1892E0A1-321C-42FB-98EE-5C3DFE509113','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union


--信息维护-职务信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','C1F7782E-7479-41FB-BE79-3466CAEE53D1','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','C1F7782E-7479-41FB-BE79-3466CAEE53D1','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','C1F7782E-7479-41FB-BE79-3466CAEE53D1','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','C1F7782E-7479-41FB-BE79-3466CAEE53D1','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--信息维护-科室信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','30D7F8B6-C60C-42C0-ABA6-DC9C167D2207','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','30D7F8B6-C60C-42C0-ABA6-DC9C167D2207','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','30D7F8B6-C60C-42C0-ABA6-DC9C167D2207','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','30D7F8B6-C60C-42C0-ABA6-DC9C167D2207','41283AEB-EB9F-45DF-9A40-83F10B95063D' union

--信息维护-设备信息
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','CFA018E2-1F84-4AB8-82D9-09120C332523','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','CFA018E2-1F84-4AB8-82D9-09120C332523','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','CFA018E2-1F84-4AB8-82D9-09120C332523','3799C387-8EC3-450A-9DB5-B2C724B0C0B2' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','CFA018E2-1F84-4AB8-82D9-09120C332523','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','CFA018E2-1F84-4AB8-82D9-09120C332523','750BE12D-8589-4D18-A20F-EC7B1931A167' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','CFA018E2-1F84-4AB8-82D9-09120C332523','0D755E95-BF99-495F-9506-27BC7D8EC904' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','CFA018E2-1F84-4AB8-82D9-09120C332523','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','CFA018E2-1F84-4AB8-82D9-09120C332523','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','CFA018E2-1F84-4AB8-82D9-09120C332523','F3D8093F-3740-4575-9102-F26AA36BE804' union
--信息维护-设备类型
----select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','E8C7B9D3-14E3-4E0E-BBBA-C5BEE7503555','750BE12D-8589-4D18-A20F-EC7B1931A167' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','E8C7B9D3-14E3-4E0E-BBBA-C5BEE7503555','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','E8C7B9D3-14E3-4E0E-BBBA-C5BEE7503555','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','E8C7B9D3-14E3-4E0E-BBBA-C5BEE7503555','0D755E95-BF99-495F-9506-27BC7D8EC904' union

--信息维护-区域信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','6E75A9C1-DC26-4E09-92F2-42D1654C8419','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','6E75A9C1-DC26-4E09-92F2-42D1654C8419','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','6E75A9C1-DC26-4E09-92F2-42D1654C8419','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','6E75A9C1-DC26-4E09-92F2-42D1654C8419','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
--信息维护-区域类型信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','AC638CE6-FFA0-4B7E-B866-A36E6D28B53A','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','AC638CE6-FFA0-4B7E-B866-A36E6D28B53A','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','AC638CE6-FFA0-4B7E-B866-A36E6D28B53A','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','AC638CE6-FFA0-4B7E-B866-A36E6D28B53A','0D755E95-BF99-495F-9506-27BC7D8EC904' union
--信息维护-定位基站信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','56CA0AAD-9A0E-4B58-8E3D-350FE6993A1D','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','56CA0AAD-9A0E-4B58-8E3D-350FE6993A1D','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','56CA0AAD-9A0E-4B58-8E3D-350FE6993A1D','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','56CA0AAD-9A0E-4B58-8E3D-350FE6993A1D','93A596E2-1C59-49F8-941D-E5A06C60F5FE' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','56CA0AAD-9A0E-4B58-8E3D-350FE6993A1D','F3D8093F-3740-4575-9102-F26AA36BE804' union
--信息维护-基站控制单元
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','77442D96-A9C9-4A17-A170-D7E14216F4A5','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','77442D96-A9C9-4A17-A170-D7E14216F4A5','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','77442D96-A9C9-4A17-A170-D7E14216F4A5','750BE12D-8589-4D18-A20F-EC7B1931A167' union
--用户信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','669C0624-0315-476F-AE23-9A23BF08BA52','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','669C0624-0315-476F-AE23-9A23BF08BA52','4BFD4B20-5B4A-4DDD-9D08-013FF8C56875' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','669C0624-0315-476F-AE23-9A23BF08BA52','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','669C0624-0315-476F-AE23-9A23BF08BA52','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','669C0624-0315-476F-AE23-9A23BF08BA52','2D43F62D-5799-4D1E-A9D0-63BF4EDC2633' union
--select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','669C0624-0315-476F-AE23-9A23BF08BA52','3799C387-8EC3-450A-9DB5-B2C724B0C0B2' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','669C0624-0315-476F-AE23-9A23BF08BA52','750BE12D-8589-4D18-A20F-EC7B1931A167' union
--模块信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','D25042F3-C5EF-41A4-B8D9-1BDB0219ECFF','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','D25042F3-C5EF-41A4-B8D9-1BDB0219ECFF','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','D25042F3-C5EF-41A4-B8D9-1BDB0219ECFF','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
--快捷菜单
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','E3B1E2E8-D61B-456E-A644-25F8802DED9A','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','E3B1E2E8-D61B-456E-A644-25F8802DED9A','750BE12D-8589-4D18-A20F-EC7B1931A167' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','E3B1E2E8-D61B-456E-A644-25F8802DED9A','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
--参数设置
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','EEA6CCF5-80FC-47D7-8FF5-D5A0AE816C43','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','EEA6CCF5-80FC-47D7-8FF5-D5A0AE816C43','0d755e95-bf99-495f-9506-27bc7d8ec904' union
--操作信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','799063F1-ACBE-4F2F-A2D2-A7D36A254AEA','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
--角色权限
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9A22C361-A606-430D-A72D-368DC47E8567','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9A22C361-A606-430D-A72D-368DC47E8567','336D8E5A-FE71-48CE-A8DF-5CDB08A40C6C' union
--角色信息
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9172722B-43E3-473A-B6F9-2BC73792323D','41283AEB-EB9F-45DF-9A40-83F10B95063D' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9172722B-43E3-473A-B6F9-2BC73792323D','0D755E95-BF99-495F-9506-27BC7D8EC904' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9172722B-43E3-473A-B6F9-2BC73792323D','187BB482-F59D-4EE1-939C-3C64AC1BB36B' union
select newid(),'3F4E84EF-C1B0-49C9-8D8E-F357B5CBC084','9172722B-43E3-473A-B6F9-2BC73792323D','750BE12D-8589-4D18-A20F-EC7B1931A167' 
GO

--职务等级信息
INSERT INTO [LPBSS].[dbo].[OfficePositionLevel] VALUES  ('初级')
INSERT INTO [LPBSS].[dbo].[OfficePositionLevel] VALUES  ('中级')
INSERT INTO [LPBSS].[dbo].[OfficePositionLevel] VALUES  ('高级')

--设备类型信息
INSERT INTO [LPBSS].[dbo].[ArticlesType]([Name],[parentId]) VALUES('普通设备',-1) 

INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('全部',0)
INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('新增',1)
INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('修改',2)
INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('删除',3)
--INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('查询',4)
INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('导入',5)
INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('导出',6)
--INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('打印',7)
INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('阅读',8)
--INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('保存',9)
INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('登录',10)
INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('登出',11)
INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('信息',12)
INSERT INTO [LPBSS].[dbo].[OperateLogType] ([TypeName] ,[TypeValue]) VALUES ('其他',255)

--地图信息
INSERT INTO [LPBSS].[dbo].[Map] ([Name] ,[Path],[ParentId],[MapOrder],[IsDefault],[BeginX],[BeginY],[EndX],[EndY],[XinParent],[YinParent]) VALUES ('办公区','MainMaps',0,1,'y',null,null,null,null,null,null)
--INSERT INTO [LPBSS].[dbo].[Map] ([Name] ,[Path],[ParentId],[MapOrder],[IsDefault],[BeginX],[BeginY],[EndX],[EndY],[XinParent],[YinParent]) VALUES ('办公区','MainMap2',0,1,'n',null,null,null,null,null,null)

-------------------------------------------InitData End---------------------------------------------------------


Go
---2.0洗煤厂增加的2013-11-18
/* 同速人员报警 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[SameSpeedPersonAlarm]') and type in ('U'))
create table SameSpeedPersonAlarm
(
	id			int primary key IDENTITY(1,1)  NOT NULL,
	CardNum		varchar(50) NOT NULL,	--卡号
    worksiteid  int NULL,		--基站id
	speed		float NULL,		--速度单位 m/s
    beginTime   datetime NULL,	--开始时间
	endTime		datetime NULL,	--结束时间
	isRead		int	NULL default 0	--是否阅读
)
Go

/* 同速人员报警 2D **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[SameSpeedPersonAlarm_2D]') and type in ('U'))
create table SameSpeedPersonAlarm_2D
(
	id			int primary key IDENTITY(1,1)  NOT NULL,
	CardNum		varchar(50) NOT NULL,	--卡号
    worksiteid  int NULL,		--基站id
	speed		float NULL,		--速度单位 m/s
    beginTime   datetime NULL,	--开始时间
	endTime		datetime NULL,	--结束时间
	isRead		int	NULL default 0	--是否阅读
)
Go

/* 等速报警基准值 **/
if not exists (select * from Settings
          where  item = 'ValueBaseSpeed')
insert into Settings
select 'ValueBaseSpeed','5','同速报警基准比较值单位m/s'
Go

/* 车辆 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[vehicles]') and type in ('U'))
create table vehicle
(
	id			int primary key IDENTITY(1,1) NOT NULL,
	cardNumber  int NOT	NULL,		--卡号
	Tag			varchar(50)	 NULL,	--编号
--	name		nvarchar(50) NULL,	--名称
--	workArea	nvarchar(50) NULL, --工作区
	registerTime datetime NULL,
	endTime      datetime NULL
)
Go

/* 巡检点设置 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[CheckPatrol]') and type in ('U'))
create table CheckPatrol
(
	id		int primary key IDENTITY(1,1) NOT NULL,
	name	nvarchar(50) NOT NULL,	--名称
	x	Float NULL,
	y	Float NULL,
	z	Float NULL,
	CaptainCount	int	NOT NULL default 0, -- 队干检查次数
	ForemanCount	int	NOT NULL default 0 -- 工长检查次数
)
Go

/* 巡检时间 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[CheckPatrolDateTimeSpan]') and type in ('U'))
create table CheckPatrolDateTimeSpan
(
	id		int primary key IDENTITY(1,1) NOT NULL,
	checkPatrolId	int NOT NULL,	--巡检id
	sTime			datetime NULL,
	eTime			dateTime NULL
)
Go

/* 巡检分析查询 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[CheckPatrolAnalysis]') and type in ('U'))
create table CheckPatrolAnalysis
(
	id		int primary key IDENTITY(1,1) NOT NULL,
    cardNumber		int NOT NULL,	--人员卡号
	checkPatrolId	int NOT NULL,	--巡检id
	occTime			datetime NOT NULL
)
Go

/* 巡检报警 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[CheckPatrolAlarm]') and type in ('U'))
create table CheckPatrolAlarm
(
	id				int primary key IDENTITY(1,1) NOT NULL,
	checkPatrolId	int NOT NULL,	--巡检id
	patrolCount		int NULL,
	duties			nvarchar(50) NULL,		-- 职务名
	isRead			int	NULL default 0		-- 0否  1是
)
Go

/* 脱岗设置 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[BreakawayPosition]') and type in ('U'))
create table BreakawayPosition
(
	id			int primary key IDENTITY(1,1) NOT NULL,
	areaId		int NOT NULL,
	beginTime	datetime  NOT NULL,
	endTime		datetime  NOT NULL,
	orderId		int NULL,			--同一区域排序Id
	duration	int	NULL			--时长(分钟)
)
Go

/* 脱岗报警 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[BreakawayPositionAlarm]') and type in ('U'))
create table BreakawayPositionAlarm
(
	id			int primary key IDENTITY(1,1) NOT NULL,
	areaId		int NOT NULL,
	beginTime	datetime  NULL,
	endTime		datetime  NULL,
	isRead		int	NULL default 0 -- 0否  1是
)
Go

if not exists(select * from syscolumns where id=object_id('[dbo].[person]') and name='birthday')
	alter table dbo.person add birthday datetime null;
--if not exists(select * from syscolumns where id=object_id('[dbo].[person]') and name='TypeWorkId')
--	alter table dbo.person add TypeWorkId datetime null;
if not exists(select * from syscolumns where id=object_id('[dbo].[officePosition]') and name='PatrolCount')
	alter table dbo.officePosition add PatrolCount int null;
if not exists(select * from [dbo].[Settings] where item = 'IsSameSpeedPersonAlarm')
	insert into Settings(Item,value,Description) select 'IsSameSpeedPersonAlarm','1', '1则启动等速报警'
if not exists(select * from [dbo].[Settings] where item = 'IsCheckPatrolAlarm')
	insert into Settings(Item,value,Description) select 'IsCheckPatrolAlarm','1', '1则启动巡检报警'
if not exists(select * from [dbo].[Settings] where item = 'IsBreakawayPositionAlarm')
	insert into Settings(Item,value,Description) select 'IsBreakawayPositionAlarm','1', '1则启动脱岗报警'
Go

/* 无信号报警设置 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[NoSignalAlarmConfig]') and type in ('U'))
create table NoSignalAlarmConfig
(
	id			int primary key IDENTITY(1,1) NOT NULL,
	startWorksiteId		int NOT NULL,		-- 开始基站
	endWorksiteId		int NOT NULL		-- 结束边界基站
)
Go

/* 无信号报警 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[NoSignalAlarm]') and type in ('U'))
create table NoSignalAlarm
(
	id			int primary key IDENTITY(1,1) NOT NULL,
	cardNumber		int NOT NULL,		-- 卡号
	WorksiteId		int NOT NULL,		-- 基站
	occTime			datetime NOT NULL,	-- 报警时间
	isRead			int NULL default 0
)
Go

/* 报警阅读时间Msg **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[AlarmReadMsg]') and type in ('U'))
CREATE TABLE AlarmReadMsg
(
	id			int primary key IDENTITY(1,1) NOT NULL,
	Msg			text NOT NULL,
	Type		int NOT NULL,
	Reader		nvarchar(50) NOT NULL,
	Remark		text  NOT NULL,
	ReadTime	datetime NOT NULL
)
Go

/* 区域经过考勤 **/
if not exists (select * from sysobjects
          where  id = object_id('[dbo].[AreaPathAttendance]') and type in ('U'))
CREATE TABLE AreaPathAttendance
(
	id			int primary key IDENTITY(1,1) NOT NULL,
	CardNumber	int	NOT NULL,
	AreaId		int NOT NULL,
	enterWorkSiteId	int NOT NULL,
	enterTime	datetime NOT NULL,
	outTime		datetime NULL
)
Go

---统计车辆数
Create procedure [dbo].[up_statisticsVehicleNum]
(
  @timeOut int	---设置时间间隔 
)
as
	--- 基站下车辆统计
	declare @worksiteStatistic table
	(
		worksiteId int,
		worksiteAddress nvarchar(50),
		VehicleNum  int 
	)
	--- 区域下车辆统计
	declare @areaStatistic table
	(
		areaId		int,
		areaName    nvarchar(50),
		VehicleNum	int  
	)
begin
	set nocount on;
	select * into #assayloc from 
	( SELECT   dbo.vehicle.id,dbo.vehicle.cardNumber,dbo.vehicle.Tag,
			 dbo.Area.aName, ws.Address, cl.OccTime, ws.WorkSiteId,dbo.Area.id AS areaId
	FROM     dbo.CardLoc AS cl INNER JOIN dbo.vehicle ON dbo.vehicle.cardNumber = cl.CardNum AND cl.CardType = 3 
				LEFT OUTER JOIN  dbo.WorkSite AS ws ON ws.WorkSiteId = cl.WorkSiteId LEFT OUTER JOIN
				dbo.corAreaWorksite AS caw ON ws.WorkSiteId = caw.WorksiteId LEFT OUTER JOIN
				dbo.Area ON dbo.Area.id = caw.areaId
	) x
	where datediff(s,occtime,getdate()) < @timeOut

---按基站统计车辆
	insert into @worksiteStatistic
	select worksiteid,address, (select count(1) from #assayloc rtVehicle 
					where rtVehicle.worksiteid = worksite.worksiteid) VehicleCount
	from worksite order by worksiteid

---按区域统计车辆
	insert into @areaStatistic 
	select area.id,area.aName,
	(select count(1) from #assayloc rtVehicle where rtVehicle.areaid = area.id or rtVehicle.areaid in 
	(select id from area a where a.parentId = area.id) or
	 rtVehicle.areaid in (select id from area a1 where a1.parentId in 
	(select Id from area a2 where a2.parentId = area.id))) personCount 
	from area order by area.id

	select worksiteid,VehicleNum from @worksiteStatistic
	select areaName,VehicleNum from @areaStatistic

	drop table #assayloc
end

GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[RssiWeightNew](
	[rssi] [int] NOT NULL,
	[weight] [float] NOT NULL,
 CONSTRAINT [PK_RssiWeightNew] PRIMARY KEY CLUSTERED 
(
	[rssi] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[RefPoints](
	[ID] [int] IDENTITY(1,1) NOT NULL,
	[Name] [nvarchar](50) NOT NULL,
	[Description] [nvarchar](256) NULL,
	[X] [float] NOT NULL,
	[Y] [float] NOT NULL,
	[Z] [float] NOT NULL,
	[InDBTime] [datetime] NOT NULL,
 CONSTRAINT [PK_RefPoints] PRIMARY KEY CLUSTERED 
(
	[ID] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

ALTER TABLE [dbo].[RefPoints] ADD  CONSTRAINT [DF_RefPoints_Z]  DEFAULT ((0)) FOR [Z]
GO

ALTER TABLE [dbo].[RefPoints] ADD  CONSTRAINT [DF_RefPoints_Indbtime]  DEFAULT (getdate()) FOR [InDBTime]
GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[RadioMap](
	[RefID] [int] NOT NULL,
	[SiteNum1] [int] NULL,
	[Rssi1] [int] NULL,
	[SiteNum2] [int] NULL,
	[Rssi2] [int] NULL,
	[SiteNum3] [int] NULL,
	[Rssi3] [int] NULL,
	[SiteNum4] [int] NULL,
	[Rssi4] [int] NULL,
	[SiteNum5] [int] NULL,
	[Rssi5] [int] NULL,
	[OccTime] [datetime] NOT NULL
) ON [PRIMARY]

GO

--初始数据
--RssiWeight
insert into RssiWeightNew(rssi,weight) values(-100,21);
insert into RssiWeightNew(rssi,weight) values(-99,22);
insert into RssiWeightNew(rssi,weight) values(-98,23);
insert into RssiWeightNew(rssi,weight) values(-97,24);
insert into RssiWeightNew(rssi,weight) values(-96,25);
insert into RssiWeightNew(rssi,weight) values(-95,26);
insert into RssiWeightNew(rssi,weight) values(-94,27);
insert into RssiWeightNew(rssi,weight) values(-93,28);
insert into RssiWeightNew(rssi,weight) values(-92,29);
insert into RssiWeightNew(rssi,weight) values(-91,30);
insert into RssiWeightNew(rssi,weight) values(-90,31);
insert into RssiWeightNew(rssi,weight) values(-89,32);
insert into RssiWeightNew(rssi,weight) values(-88,34);
insert into RssiWeightNew(rssi,weight) values(-87,35);
insert into RssiWeightNew(rssi,weight) values(-86,36);
insert into RssiWeightNew(rssi,weight) values(-85,38);
insert into RssiWeightNew(rssi,weight) values(-84,39);
insert into RssiWeightNew(rssi,weight) values(-83,41);
insert into RssiWeightNew(rssi,weight) values(-82,42);
insert into RssiWeightNew(rssi,weight) values(-81,44);
insert into RssiWeightNew(rssi,weight) values(-80,46);
insert into RssiWeightNew(rssi,weight) values(-79,48);
insert into RssiWeightNew(rssi,weight) values(-78,50);
insert into RssiWeightNew(rssi,weight) values(-77,52);
insert into RssiWeightNew(rssi,weight) values(-76,54);
insert into RssiWeightNew(rssi,weight) values(-75,56);
insert into RssiWeightNew(rssi,weight) values(-74,58);
insert into RssiWeightNew(rssi,weight) values(-73,60);
insert into RssiWeightNew(rssi,weight) values(-72,63);
insert into RssiWeightNew(rssi,weight) values(-71,65);
insert into RssiWeightNew(rssi,weight) values(-70,68);
insert into RssiWeightNew(rssi,weight) values(-69,70);
insert into RssiWeightNew(rssi,weight) values(-68,73);
insert into RssiWeightNew(rssi,weight) values(-67,76);
insert into RssiWeightNew(rssi,weight) values(-66,79);
insert into RssiWeightNew(rssi,weight) values(-65,82);
insert into RssiWeightNew(rssi,weight) values(-64,85);
insert into RssiWeightNew(rssi,weight) values(-63,89);
insert into RssiWeightNew(rssi,weight) values(-62,92);
insert into RssiWeightNew(rssi,weight) values(-61,96);
insert into RssiWeightNew(rssi,weight) values(-60,100);
insert into RssiWeightNew(rssi,weight) values(-59,103);
insert into RssiWeightNew(rssi,weight) values(-58,107);
insert into RssiWeightNew(rssi,weight) values(-57,112);
insert into RssiWeightNew(rssi,weight) values(-56,116);
insert into RssiWeightNew(rssi,weight) values(-55,121);
insert into RssiWeightNew(rssi,weight) values(-54,125);
insert into RssiWeightNew(rssi,weight) values(-53,130);
insert into RssiWeightNew(rssi,weight) values(-52,135);
insert into RssiWeightNew(rssi,weight) values(-51,141);
insert into RssiWeightNew(rssi,weight) values(-50,146);
insert into RssiWeightNew(rssi,weight) values(-49,152);
insert into RssiWeightNew(rssi,weight) values(-48,158);
insert into RssiWeightNew(rssi,weight) values(-47,164);
insert into RssiWeightNew(rssi,weight) values(-46,171);
insert into RssiWeightNew(rssi,weight) values(-45,177);
insert into RssiWeightNew(rssi,weight) values(-44,184);
insert into RssiWeightNew(rssi,weight) values(-43,192);
insert into RssiWeightNew(rssi,weight) values(-42,199);
insert into RssiWeightNew(rssi,weight) values(-41,207);
insert into RssiWeightNew(rssi,weight) values(-40,215);
insert into RssiWeightNew(rssi,weight) values(-39,223);
insert into RssiWeightNew(rssi,weight) values(-38,232);
insert into RssiWeightNew(rssi,weight) values(-37,241);
insert into RssiWeightNew(rssi,weight) values(-36,251);
insert into RssiWeightNew(rssi,weight) values(-35,261);
insert into RssiWeightNew(rssi,weight) values(-34,271);
insert into RssiWeightNew(rssi,weight) values(-33,281);
insert into RssiWeightNew(rssi,weight) values(-32,292);
insert into RssiWeightNew(rssi,weight) values(-31,304);
insert into RssiWeightNew(rssi,weight) values(-30,316);
insert into RssiWeightNew(rssi,weight) values(-29,328);
insert into RssiWeightNew(rssi,weight) values(-28,341);
insert into RssiWeightNew(rssi,weight) values(-27,354);
insert into RssiWeightNew(rssi,weight) values(-26,368);
insert into RssiWeightNew(rssi,weight) values(-25,383);
insert into RssiWeightNew(rssi,weight) values(-24,398);
insert into RssiWeightNew(rssi,weight) values(-23,413);
insert into RssiWeightNew(rssi,weight) values(-22,429);
insert into RssiWeightNew(rssi,weight) values(-21,446);
insert into RssiWeightNew(rssi,weight) values(-20,464);
insert into RssiWeightNew(rssi,weight) values(-19,482);
insert into RssiWeightNew(rssi,weight) values(-18,501);
insert into RssiWeightNew(rssi,weight) values(-17,520);
insert into RssiWeightNew(rssi,weight) values(-16,541);
insert into RssiWeightNew(rssi,weight) values(-15,562);
insert into RssiWeightNew(rssi,weight) values(-14,584);
insert into RssiWeightNew(rssi,weight) values(-13,607);
insert into RssiWeightNew(rssi,weight) values(-12,630);
insert into RssiWeightNew(rssi,weight) values(-11,655);
insert into RssiWeightNew(rssi,weight) values(-10,681);
insert into RssiWeightNew(rssi,weight) values(-9,707);
insert into RssiWeightNew(rssi,weight) values(-8,735);
insert into RssiWeightNew(rssi,weight) values(-7,764);
insert into RssiWeightNew(rssi,weight) values(-6,794);
insert into RssiWeightNew(rssi,weight) values(-5,825);
insert into RssiWeightNew(rssi,weight) values(-4,857);
insert into RssiWeightNew(rssi,weight) values(-3,891);
insert into RssiWeightNew(rssi,weight) values(-2,926);
insert into RssiWeightNew(rssi,weight) values(-1,962);
GO

/****** Object:  StoredProcedure [dbo].[sp_CreatePartition_2D]    Script Date: 06/05/2014 18:04:25 ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

	
CREATE proc [dbo].[sp_CreatePartition_2D]
(
	@dbName nvarchar(50),   --数据库名  LPBSS
	@tableName nvarchar(50) --系统当前时间 yyyyMMdd
)
as

IF OBJECT_ID(@tableName) IS NOT NULL 
BEGIN
    RETURN 
END

DECLARE @filePath nvarchar(50),--分区文件存放路径
        @employeeCount int,--员工总数
		@fileNameFormart nvarchar(50),--分区文件名称
		@curIndex int,  --循环索引，初始化为0
		@partitionFileSql nvarchar(max),--创建分区相关
		@partitionFuncSqlFormart nvarchar(max),--分区函数结构
		@partitionFuncSql nvarchar(max),--分区函数参数
		@partitionSchemeFormart nvarchar(max),--分区架构语句
		@partitionSchemeSql nvarchar(max), --分区架构sql语句
		@partitionFuncName nvarchar(50), --分区函数名称
		@partitionSchemeName nvarchar(50), --分区架构名称		
		@createLocTableSql nvarchar(max)--创建日期表sql

SELECT @filePath=Value FROM Settings WHERE Item='FileGroupPath'
set @employeeCount=10
set @fileNameFormart=N'_'
set @curIndex=0;
set @partitionFuncSql=''
set @partitionSchemeSql=''
set @partitionFuncName=@tableName+N'_Fun_Loc_Id';
set @partitionSchemeName=@tableName+N'_Sch_Loc_Id';

set @partitionFuncSqlFormart=N'CREATE PARTITION FUNCTION  '
                             +@partitionFuncName+'(int) AS '+
							 N'RANGE RIGHT '+ N'FOR VALUES('
set @partitionSchemeFormart=N'CREATE PARTITION SCHEME '+
                            @partitionSchemeName+N' AS  '+
							N' PARTITION '+@partitionFuncName+N' TO(';

while(@curIndex<@employeeCount)
	begin
	   
	   declare @flName nvarchar(50)
	   set @flName=@fileNameFormart+@tableName+N'_'+CONVERT(nvarchar(50),@curIndex)
	   
	   IF EXISTS ( SELECT 1 FROM sysfilegroups WHERE groupname = @flName )
	       RETURN

	   --添加分区文件组至数据库
	   set @partitionFileSql=N'ALTER DATABASE '+@dbName+' ADD FILEGROUP ['+@flName+'] '
	   exec sp_executesql @partitionFileSql;
	   
	   --创建分区文件
	   set @partitionFileSql=N' ALTER DATABASE '+@dbName+N' ADD FILE'+N' (NAME = N'''+@flName
	   +'_Data'',FILENAME = N'''+@filePath+@flName+'.ndf'',SIZE = 5MB, FILEGROWTH = 10% )'+
		N' TO FILEGROUP ['+@flName+']';
	   exec sp_executesql @partitionFileSql
	    
	    IF @curIndex > 0
			set	@partitionFuncSql=@partitionFuncSql+N''+CONVERT(nvarchar(50),@curIndex)+',';
		set @partitionSchemeSql=@partitionSchemeSql+@flName+N',';
	    set @curIndex=@curIndex+1;
	end

   set @partitionFuncSql=substring(@partitionFuncSql,0,len(@partitionFuncSql));
   set @partitionFuncSqlFormart=@partitionFuncSqlFormart+@partitionFuncSql+')';
   exec sp_executesql @partitionFuncSqlFormart;
   
   set @partitionSchemeSql=substring(@partitionSchemeSql,0,len(@partitionSchemeSql));
   set @partitionSchemeFormart=@partitionSchemeFormart+@partitionSchemeSql+N')';
   exec sp_executesql @partitionSchemeFormart;

   SET @createLocTableSql = N'CREATE TABLE [dbo].[' + @tableName + '](
								CardNum nvarchar(50) NOT NULL,
								CardType  int NOT NULL,
								WorkSiteId int NOT NULL, 
								x  float NOT NULL,
								y  float NOT NULL,
								Speed  float NULL,
								OccTime dateTime NOT NULL, 
								IsOnWorkSite int NOT NULL,
								ObjectId int NOT NULL,
								CardModel int default(0)
								) on '+@partitionSchemeName+'(CardModel)';
								
	exec sp_executesql @createLocTableSql;
	
GO

/****** Object:  StoredProcedure [dbo].[sp_CreatePartition_3D]    Script Date: 06/05/2014 18:04:53 ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

	
CREATE proc [dbo].[sp_CreatePartition_3D]
(
	@dbName nvarchar(50),   --数据库名  LPBSS
	@tableName nvarchar(50) --系统当前时间 yyyyMMdd
)
as

IF OBJECT_ID(@tableName) IS NOT NULL 
BEGIN
    RETURN 
END

DECLARE @filePath nvarchar(50),--分区文件存放路径
        @employeeCount int,--员工总数
		@fileNameFormart nvarchar(50),--分区文件名称
		@curIndex int,  --循环索引，初始化为0
		@partitionFileSql nvarchar(max),--创建分区相关
		@partitionFuncSqlFormart nvarchar(max),--分区函数结构
		@partitionFuncSql nvarchar(max),--分区函数参数
		@partitionSchemeFormart nvarchar(max),--分区架构语句
		@partitionSchemeSql nvarchar(max), --分区架构sql语句
		@partitionFuncName nvarchar(50), --分区函数名称
		@partitionSchemeName nvarchar(50), --分区架构名称		
		@createLocTableSql nvarchar(max)--创建日期表sql

SELECT @filePath=Value FROM Settings WHERE Item='FileGroupPath'
set @employeeCount=10
set @fileNameFormart=N'_'
set @curIndex=0;
set @partitionFuncSql=''
set @partitionSchemeSql=''
set @partitionFuncName=@tableName+N'_Fun_Loc_Id';
set @partitionSchemeName=@tableName+N'_Sch_Loc_Id';

set @partitionFuncSqlFormart=N'CREATE PARTITION FUNCTION  '
                             +@partitionFuncName+'(int) AS '+
							 N'RANGE RIGHT '+ N'FOR VALUES('
set @partitionSchemeFormart=N'CREATE PARTITION SCHEME '+
                            @partitionSchemeName+N' AS  '+
							N' PARTITION '+@partitionFuncName+N' TO(';

while(@curIndex<@employeeCount)
	begin
	   
	   declare @flName nvarchar(50)
	   set @flName=@fileNameFormart+@tableName+N'_'+CONVERT(nvarchar(50),@curIndex)
	   
	   IF EXISTS ( SELECT 1 FROM sysfilegroups WHERE groupname = @flName )
	       RETURN

	   --添加分区文件组至数据库
	   set @partitionFileSql=N'ALTER DATABASE '+@dbName+' ADD FILEGROUP ['+@flName+'] '
	   exec sp_executesql @partitionFileSql;
	   
	   --创建分区文件
	   set @partitionFileSql=N' ALTER DATABASE '+@dbName+N' ADD FILE'+N' (NAME = N'''+@flName
	   +'_Data'',FILENAME = N'''+@filePath+@flName+'.ndf'',SIZE = 5MB, FILEGROWTH = 10% )'+
		N' TO FILEGROUP ['+@flName+']';
	   exec sp_executesql @partitionFileSql
	    
	    IF @curIndex > 0
			set	@partitionFuncSql=@partitionFuncSql+N''+CONVERT(nvarchar(50),@curIndex)+',';
		set @partitionSchemeSql=@partitionSchemeSql+@flName+N',';
	    set @curIndex=@curIndex+1;
	end

   set @partitionFuncSql=substring(@partitionFuncSql,0,len(@partitionFuncSql));
   set @partitionFuncSqlFormart=@partitionFuncSqlFormart+@partitionFuncSql+')';
   exec sp_executesql @partitionFuncSqlFormart;
   
   set @partitionSchemeSql=substring(@partitionSchemeSql,0,len(@partitionSchemeSql));
   set @partitionSchemeFormart=@partitionSchemeFormart+@partitionSchemeSql+N')';
   exec sp_executesql @partitionSchemeFormart;

   SET @createLocTableSql = N'CREATE TABLE [dbo].[' + @tableName + '](
								CardNum nvarchar(50) NOT NULL,
								CardType  int NOT NULL,
								WorkSiteId int NOT NULL, 
								x  float NOT NULL,
								y  float NOT NULL,
								z  float NOT NULL,
								Speed  float NULL,
								OccTime dateTime NOT NULL, 
								CardModel int default(0)
								) on '+@partitionSchemeName+'(CardModel)';
								
	exec sp_executesql @createLocTableSql;
	
GO

/****** Object:  StoredProcedure [dbo].[sp_DropPartition]    Script Date: 06/05/2014 18:05:17 ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO


-- ================================================
-- 删除精确定位数据表
-- 1:删除表 2:删除数据文件 3:删除文件组
-- ================================================
CREATE PROCEDURE [dbo].[sp_DropPartition] 
( 
@dbName VARCHAR(50),
@tableName VARCHAR(50) 
)
AS 
	DECLARE @sql VARCHAR(1024)
	IF OBJECT_ID(@tableName) IS NOT NULL 
	BEGIN
		SET @sql = 'drop table [dbo].[' + @tableName + ']'
		EXEC(@sql)    --删除分区表
	END
    
	DECLARE @schemeName VARCHAR(64)
	SET @schemeName = @tableName + '_Sch_Loc_Id'
	IF EXISTS(SELECT 1 FROM sys.partition_schemes WHERE name = @schemeName)
	BEGIN
		SET @sql = 'DROP PARTITION SCHEME ' + @schemeName
		EXEC(@sql)    --删除分区计划
	END
    
	DECLARE @funName VARCHAR(64)
	SET @funName = @tableName + '_Fun_Loc_Id'
	IF EXISTS(SELECT 1 FROM sys.partition_functions WHERE name = @funName)
	BEGIN
		SET @sql = 'DROP PARTITION FUNCTION ' + @funName
		EXEC(@sql)    --删除分区函数
	END
    
	DECLARE @fileName nvarchar(64), @fileGroupName nvarchar(64), @index INT
	SET @index = 0
	WHILE @index < 10
	BEGIN
		set @fileGroupName = N'_' + @tableName + N'_' + CONVERT(nvarchar(8), @index)
		SET @fileName = @fileGroupName + N'_Data'
		
		IF EXISTS(SELECT 1 FROM sysfiles WHERE name = @fileName) 
		BEGIN
			SET @sql = ' alter database ' + @dbName + ' remove file [' + @fileName + ']'  --删除文件
			EXEC(@sql)
		END
          
		IF EXISTS(SELECT 1 FROM  sysfilegroups WHERE groupname = @fileGroupName) 
		BEGIN  
			SET @sql = ' alter database ' + @dbName + ' remove filegroup [' + @fileGroupName + ']' --删除文件组
			EXEC(@sql)
		END
        
		SET @index = @index + 1
	END
GO


/****** Object:  Trigger [dbo].[trg_CameraDelete]    Script Date: 02/04/2015 17:04:07 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
Create Trigger [dbo].[trg_CameraDelete]
on [dbo].[Camera]
AFTER DELETE
AS
DELETE FROM CameraToWorkSiteMap WHERE CameraId NOT IN(SELECT id FROM Camera)

GO



/****** Object:  Trigger [dbo].[person]    Script Date: 03/26/2015 17:25:07 ******/
Create TRIGGER [dbo].[Employee_trigger_Insert_Person] ON [dbo].[person]
    After Insert
AS
begin 
declare @personNo int;  --员工id
 select @personNo = personNo from inserted
 if @personNo is not null
	insert into Employee(PersonId,Number,DepartmentId,ClassTeamId,OfficePositionId) 
	select id,personNo,DepartmentId,ClassTeamId,OfficePositionId from inserted
end
Go

Create TRIGGER [dbo].[Employee_trigger_Update_Person] ON [dbo].[person]
    After Update
AS
begin
declare @id int;
declare @Number varchar(50);
declare @DepartmentId int;
declare @ClassTeamId int;
declare @OfficePositionId int;
 select @id = id, @Number = personNo, @DepartmentId = DepartmentId,@ClassTeamId = ClassTeamId,@OfficePositionId = OfficePositionId 
 from [person]
 if @Number is not null
 update Employee set Number=@Number ,DepartmentId = @DepartmentId,ClassTeamId = @ClassTeamId,OfficePositionId = @OfficePositionId
 where PersonId = @id
end

Go

/****** Object:  Table [dbo].[CardInfoHistory]    Script Date: 04/17/2015 14:53:53 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[CardInfoHistory](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[CardNum] [nvarchar](50) NOT NULL,
	[CardType] [int] NOT NULL,
	[Version] [nvarchar](50) NULL,
	[Power] [float] NULL,
	[Status] [int] NULL,
	[OccTime] [datetime] NULL,
	[InDBTime] [datetime] NOT NULL DEFAULT (getdate()),
 CONSTRAINT [PK_CardInfoHistory] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE SP_InsertCardInformation
(
	@CardNum NVARCHAR(50),
	@CardType INT,
	@Version NVARCHAR(50),
	@Power FLOAT,
	@Status INT,
	@OccTime DATETIME,
	@SaveHour INT -- <=0: no insert; >0: insert one per SaveHour hour
)
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- Insert statements for procedure here
    DECLARE @strpower NVARCHAR(50);
    SET @strpower = CONVERT(NVARCHAR(50),@Power)+'v';
    UPDATE dbo.CardInformation SET Power=@strpower,Status=@Status,OccTime=@OccTime WHERE CardNum=@CardNum AND CardType=@CardType;
	IF @@ROWCOUNT <= 0
		INSERT INTO dbo.CardInformation(CardNum,CardType,Version,Power,Status,OccTime) VALUES(@CardNum,@CardType,@Version,@strpower,@Status,@OccTime);
		
	IF @SaveHour <= 0
		RETURN;
	
	DECLARE @lasttime DATETIME;
	SET @lasttime = NULL;
	SELECT TOP 1 @lasttime=OccTime FROM dbo.CardInfoHistory WHERE CardNum=@CardNum AND CardType=@CardType ORDER BY OccTime DESC
	IF @lasttime IS NULL OR DATEDIFF(HOUR, @lasttime, @OccTime) >= @SaveHour
		INSERT INTO dbo.CardInfoHistory(CardNum,CardType,Version,Power,Status,OccTime) VALUES(@CardNum,@CardType,@Version,@Power,@Status,@OccTime);
END

GO

/****** Object:  TRIGGER Tr_WorkSiteAlarm_Instead_Insert    Script Date: 04/21/2015 14:53:53 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TRIGGER Tr_WorkSiteAlarm_Instead_Insert
   ON  WorkSiteAlarm 
   INSTEAD OF INSERT
AS 
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- Insert statements for trigger here
    DECLARE @siteid INT, @occtime DATETIME;
    SELECT @siteid=WorkSiteId, @occtime=OccTime FROM INSERTED;
    
	DECLARE @oktime DATETIME, @isread INT;
	SET @isread = NULL;
	SELECT TOP 1 @oktime=OkTime, @isread=IsRead FROM dbo.WorkSiteAlarm WHERE WorkSiteId=@siteid ORDER BY id DESC
	
	IF @isread IS NULL OR @oktime IS NOT NULL OR @isread=1
		INSERT INTO dbo.WorkSiteAlarm(WorkSiteId, OccTime) VALUES(@siteid, @occtime);

END

GO

/****** Object:  TRIGGER Tr_CardException_Instead_Insert    Script Date: 04/21/2015 14:53:53 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TRIGGER Tr_CardException_Instead_Insert
   ON  CardException 
   INSTEAD OF INSERT
AS 
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- Insert statements for trigger here
    DECLARE @cardnum NVARCHAR(50), @cardtype INT, @sitenum INT, @occtime DATETIME;
    SELECT @cardnum=CardNum, @cardtype=CardType, @sitenum=WorkSiteId, @occtime=OccTime FROM INSERTED;
    
    DECLARE @oktime DATETIME, @isread INT;
	SET @isread = NULL;
	SELECT TOP 1 @oktime=OkTime, @isread=IsRead FROM dbo.CardException WHERE CardNum=@cardnum AND CardType=@cardtype ORDER BY id DESC
	
	IF @isread IS NULL OR @oktime IS NOT NULL OR @isread=1
		INSERT INTO dbo.CardException(CardNum, CardType, WorkSiteId, OccTime) VALUES(@cardnum, @cardtype, @sitenum, @occtime);
END

GO
