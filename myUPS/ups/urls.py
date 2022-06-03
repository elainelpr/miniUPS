from django.urls import path
from . import views

urlpatterns = [
    path('', views.signupUsers, name='signupusers'),
    path('trackpackage', views.trackPackage, name = "track_package"),
    path('trackresult', views.trackResult, name="track_result"),
    path('home', views.home, name='ups-home'),
    path('login', views.loginUsers, name='loginusers'),
    path('search', views.search, name = "search_package"),
    path('info', views.package_info, name = "package_info"),
    path('viewpackage', views.viewPackage, name="view_package"),
    path('allpackage', views.allPackage_info, name = 'all_package'),
    path('editpackage', views.editPackage, name='edit_package'),
    path('packageupdata', views.packageUpdate, name="package_update"),
    path('searchsurvey', views.searchSurvey, name = "search_survey"),
    path('packagecomment', views.packageComment, name = "package_comment"),
    path('finishsurvey', views.finishSurvey, name="finish_survey"),
    path('surveyhistory', views.surveyHistory, name="survey_history"),
    path('recentlytracked', views.recentlyTracked, name="recently_tracked")
]