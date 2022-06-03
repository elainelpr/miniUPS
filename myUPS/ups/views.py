from django.shortcuts import render

# Create your views here.

from django.shortcuts import render, redirect
from django.http import HttpResponse
from django.contrib.auth.forms import UserCreationForm, AuthenticationForm
from django.contrib import messages
from django.contrib.auth import login
from ups.models import CreatedUsersForm
from ups.models import Package,Survey,Tracked
from django.contrib import messages
from django.contrib.auth.models import User
from django import forms


posts = [
  {
    'author': 'Elaine',
    'title': 'Welcome Back!',
    'content': 'Keep growing your small business with big savings on shipping.',
    'date_posted': 'April 17, 2022'
  },
  {
    'author': 'Thomas',
    'title': 'Need Shipping Supplies?',
    'content': 'Get free shipping supplies, like packaging, forms and labels, just by logging into UPS.com®',
    'date_posted': 'April 16, 2022'
  },
  {
    'author': 'Tony',
    'title': 'About Us',
    'content': 'Explore what makes UPS unique',
    'date_posted': 'April 15, 2022'  
  }
]


def trackPackage(request):
  context = {"info":"Welcome"}
  return render(request, 'ups/track_package.html', context)

def trackResult(request):
  try:
    package_num = request.POST['packageid']
    ob_package = Package.objects.filter(packageid=package_num)
    print(package_num)
    if(ob_package.exists()):
      context = {'ob_package': ob_package[0]}
      return render(request, "ups/trackPackage_info.html", context) #加载模板
    else:
      messages.error(request, "Sorry, the tracking number is wrong")
      return redirect('track_package') 
  except:
    context = {"info":"Sorry, Please try it again"}
    return render(request, "ups/track.html", context)
        

def home(request):
  context = {'posts': posts}
  return render(request, 'ups/home.html', context)

def signupUsers(request):
    if request.method == 'POST':
        form = CreatedUsersForm(request.POST)
        if form.is_valid():
            form.save()
            username = form.cleaned_data.get('username')
            return redirect("loginusers")
    else:
        form = CreatedUsersForm()
    return render(request, "ups/sign_up.html", {'form': form})
  
def loginUsers(request):
  if request.method == 'POST':
    form = AuthenticationForm(data=request.POST)
    if form.is_valid():
      login(request, form.get_user())
      return redirect("ups-home")
  else:
    form = AuthenticationForm()
  return render(request, 'ups/login.html', {'form':form})


def search(request):
  return render(request, "ups/search_package.html")

def package_info(request):
  try:
    buyerID = request.session.get('_auth_user_id')
    package_num = request.POST['packageid']
    ob_package = Package.objects.filter(user_id=buyerID, packageid=package_num)  #tracking number符合
    if ob_package.exists():
          recent_package = Tracked()
          recent_package.package_id = package_num
          recent_package.userx = ob_package[0].userx
          recent_package.usery = ob_package[0].usery
          recent_package.status = ob_package[0].status
          recent_package.user_id = buyerID
          recent_package.save()
          context = {'ob_package': ob_package[0]}
          return render(request, "ups/package_info.html", context) #加载模板
    else:
          messages.error(request, "Sorry, the tracking number is wrong")
          return redirect('search_package') 
  except:
    context = {"info":"Sorry, Please try it again"}
    return render(request, "ups/package_base.html", context)
  
def viewPackage(request):
  try:
    buyerid = request.session.get('_auth_user_id')
    ob_package = Package.objects.filter(user_id=buyerid)
    context = {"packagelist": ob_package}
    return render(request, "ups/view_package.html", context) #加载模板
  except:
    return HttpResponse("Can't find package information...")

def allPackage_info(request):
  package_id = request.POST['packagelist']
  ob_package = Package.objects.get(packageid = package_id)
  
  context = {'ob_package': ob_package}
  return render(request, 'ups/package_info.html', context)

def editPackage(request):
  try:
    package_id = request.POST['ob_package']
    package_info = Package.objects.get(packageid = package_id)
    context = {'package_info':package_info}
    return render(request,"ups/edit_package.html",context)
  except:
    return HttpResponse("Cannot find the package infomation...")

def packageUpdate(request):
  try:
    package_id = request.POST['package_info']
    ob_package = Package.objects.get(packageid=package_id)
    print(ob_package.status)
    if(ob_package.status<3):     
      ob_package.userx = request.POST['userx']
      ob_package.usery = request.POST['usery']
      ob_package.change = 1
      ob_package.save()
      context = {'ob_package':ob_package}
      return render(request,"ups/package_info.html",context)
    else:
      messages.error(request, "Sorry, you cannot change the address")
      return render(request,"ups/edit_package.html")
  except:
      package_id = request.POST['package_info']
      ob_package = Package.objects.get(packageid=package_id)
      context = {'ob_package':ob_package}
      return render(request,"ups/edit_package.html",context)

def searchSurvey(request):
  try:
    buyerid = request.session.get('_auth_user_id')
    ob_package = Package.objects.filter(user_id=buyerid, status=5)
    context = {"packagelist": ob_package}
    return render(request, "ups/search_survey.html", context) #加载模板
  except:
    return HttpResponse("Can't find package information...")
  
def packageComment(request):
  package_id = request.POST['packagelist']
  ob_package = Package.objects.get(packageid = package_id)
  context = {'ob_package': ob_package}
  return render(request, "ups/package_comment.html", context)

def finishSurvey(request):
  try:
    package_id = request.POST['package_info']
    finish = Survey()
    finish.comment = request.POST['package_comment']
    finish.package_id = package_id
    finish.user_id = request.POST['buyer_id']
    finish.save()
    return redirect("search_survey")
  except:
    return HttpResponse("Please try it again...")

def surveyHistory(request):
  buyer_id = request.session.get('_auth_user_id')
  history = Survey.objects.filter(user_id=buyer_id)
  context = {'package_history': history}
  return render(request, "ups/survey_history.html", context) 

def recentlyTracked(request):
  buyer_id = request.session.get('_auth_user_id')
  tracked = Tracked.objects.filter(user_id = buyer_id)
  context = {'tracked': tracked}
  return render(request, "ups/recently_tracked.html", context)